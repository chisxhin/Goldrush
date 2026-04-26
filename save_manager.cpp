#include "save_manager.hpp"

#include "game.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <sstream>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {
struct LoadedGameState {
    RuleSet rules;
    std::vector<Player> players;
    std::vector<std::string> historyEntries;
    SerializedDeckState actionDeck;
    SerializedDeckState collegeCareerDeck;
    SerializedDeckState careerDeck;
    SerializedDeckState houseDeck;
    SerializedDeckState investDeck;
    SerializedDeckState petDeck;
    bool rngFixedSeed;
    std::uint32_t rngSeedValue;
    std::string rngState;
    int currentPlayerIndex;
    int turnCounter;
    int retiredCount;
    std::string gameId;
    std::string assignedFilename;
    std::time_t createdTime;
    std::time_t lastSavedTime;
};

bool fillLocalTime(std::time_t timestamp, std::tm& out) {
#if defined(_WIN32)
    return localtime_s(&out, &timestamp) == 0;
#else
    return localtime_r(&timestamp, &out) != 0;
#endif
}

fs::path saveDirectoryPath() {
    return fs::path("saves");
}

bool hasSavExtension(const std::string& filename) {
    return filename.size() >= 4 && filename.substr(filename.size() - 4) == ".sav";
}

std::string ensureSavExtension(const std::string& filename) {
    if (filename.empty() || hasSavExtension(filename)) {
        return filename;
    }
    return filename + ".sav";
}

std::time_t fileTimeToTimeT(const fs::file_time_type& value) {
    const std::chrono::system_clock::time_point systemPoint =
        std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            value - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(systemPoint);
}

std::string formatTimestamp(std::time_t timestamp, const char* pattern) {
    std::tm localTime;
    if (!fillLocalTime(timestamp, localTime)) {
        return "unknown";
    }

    char buffer[64] = {0};
    if (std::strftime(buffer, sizeof(buffer), pattern, &localTime) == 0) {
        return "unknown";
    }
    return buffer;
}

std::string escapeField(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        switch (c) {
            case '\\':
                escaped += "\\\\";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(c);
                break;
        }
    }
    return escaped;
}

bool unescapeField(const std::string& value, std::string& decoded) {
    decoded.clear();
    decoded.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        if (c != '\\') {
            decoded.push_back(c);
            continue;
        }

        if (i + 1 >= value.size()) {
            return false;
        }

        ++i;
        switch (value[i]) {
            case '\\':
                decoded.push_back('\\');
                break;
            case 'n':
                decoded.push_back('\n');
                break;
            case 'r':
                decoded.push_back('\r');
                break;
            case 't':
                decoded.push_back('\t');
                break;
            default:
                return false;
        }
    }
    return true;
}

std::vector<std::string> splitTabbed(const std::string& line) {
    std::vector<std::string> parts;
    std::string current;

    for (std::size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '\t') {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(line[i]);
        }
    }
    parts.push_back(current);
    return parts;
}

bool parseInt(const std::string& text, int& value) {
    std::istringstream in(text);
    in >> value;
    return !in.fail() && in.eof();
}

bool parseUInt(const std::string& text, std::uint32_t& value) {
    std::istringstream in(text);
    unsigned long parsed = 0;
    in >> parsed;
    if (in.fail() || !in.eof()) {
        return false;
    }
    value = static_cast<std::uint32_t>(parsed);
    return true;
}

bool parseTimeValue(const std::string& text, std::time_t& value) {
    std::istringstream in(text);
    long long parsed = 0;
    in >> parsed;
    if (in.fail() || !in.eof()) {
        return false;
    }
    value = static_cast<std::time_t>(parsed);
    return true;
}

bool parseBool(const std::string& text, bool& value) {
    int parsed = 0;
    if (!parseInt(text, parsed)) {
        return false;
    }
    value = parsed != 0;
    return true;
}

std::string boolText(bool value) {
    return value ? "1" : "0";
}

void finalizeSaveFileInfo(SaveFileInfo& info) {
    if (info.assignedFilename.empty()) {
        info.assignedFilename = info.filename;
    } else {
        const fs::path assignedPath(info.assignedFilename);
        if (assignedPath.has_filename()) {
            info.assignedFilename = ensureSavExtension(assignedPath.filename().string());
        } else {
            info.assignedFilename = ensureSavExtension(info.assignedFilename);
        }
    }
    if (info.createdTime <= 0) {
        info.createdTime = info.modifiedTime;
    }
    if (info.lastSavedTime <= 0) {
        info.lastSavedTime = info.modifiedTime;
    }

    info.createdText = formatTimestamp(info.createdTime, "%Y-%m-%d %H:%M:%S");
    info.lastSavedText = formatTimestamp(info.lastSavedTime, "%Y-%m-%d %H:%M:%S");
    info.assignedTarget = info.assignedFilename == info.filename;
}

bool readSaveFileMetadata(const fs::path& path, SaveFileInfo& info) {
    std::ifstream in(path.c_str());
    if (!in) {
        return false;
    }

    std::string line;
    if (!std::getline(in, line)) {
        return false;
    }

    std::vector<std::string> headerParts = splitTabbed(line);
    if (headerParts.size() != 2 || headerParts[0] != "GOLDRUSH_SAVE") {
        return false;
    }

    int version = 0;
    if (!parseInt(headerParts[1], version) || version < 1 || version > SaveManager::SAVE_VERSION) {
        return false;
    }

    info.saveVersion = version;

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> parts = splitTabbed(line);
        for (std::size_t i = 0; i < parts.size(); ++i) {
            std::string decoded;
            if (!unescapeField(parts[i], decoded)) {
                return false;
            }
            parts[i] = decoded;
        }

        if (parts.empty()) {
            continue;
        }
        if (parts[0] == "END") {
            break;
        }

        if (parts[0] == "GAME" && parts.size() >= 3) {
            if (parts[1] == "ID") {
                info.gameId = parts[2];
            } else if (parts[1] == "CREATED_AT") {
                if (!parseTimeValue(parts[2], info.createdTime)) {
                    return false;
                }
            } else if (parts[1] == "LAST_SAVED_AT") {
                if (!parseTimeValue(parts[2], info.lastSavedTime)) {
                    return false;
                }
            } else if (parts[1] == "ASSIGNED_FILE") {
                info.assignedFilename = parts[2];
            }
        }

        if (parts[0] == "PLAYERS" ||
            parts[0] == "PLAYER" ||
            parts[0] == "HISTORY" ||
            parts[0] == "DECK") {
            break;
        }
    }

    info.metadataValid = true;
    finalizeSaveFileInfo(info);
    return true;
}

fs::path makeArchivePath(const fs::path& originalPath) {
    fs::path candidate(originalPath.string() + ".bak");
    int suffix = 1;
    std::error_code errorCode;
    while (fs::exists(candidate, errorCode)) {
        candidate = fs::path(originalPath.string() + ".bak" + std::to_string(suffix));
        ++suffix;
        errorCode.clear();
    }
    return candidate;
}

std::string deckSlotName(DeckSlot slot) {
    switch (slot) {
        case DECK_ACTION:
            return "ACTION";
        case DECK_COLLEGE_CAREER:
            return "COLLEGE_CAREER";
        case DECK_CAREER:
            return "CAREER";
        case DECK_HOUSE:
            return "HOUSE";
        case DECK_INVEST:
            return "INVEST";
        case DECK_PET:
            return "PET";
        default:
            return "UNKNOWN";
    }
}

bool parseDeckSlot(const std::string& text, DeckSlot& slot) {
    if (text == "ACTION") {
        slot = DECK_ACTION;
        return true;
    }
    if (text == "COLLEGE_CAREER") {
        slot = DECK_COLLEGE_CAREER;
        return true;
    }
    if (text == "CAREER") {
        slot = DECK_CAREER;
        return true;
    }
    if (text == "HOUSE") {
        slot = DECK_HOUSE;
        return true;
    }
    if (text == "INVEST") {
        slot = DECK_INVEST;
        return true;
    }
    if (text == "PET") {
        slot = DECK_PET;
        return true;
    }
    return false;
}

Player makeDefaultPlayer(int index) {
    Player player = Player();
    player.token = static_cast<char>('A' + index);
    player.job = "Unemployed";
    player.startChoice = -1;
    player.familyChoice = -1;
    player.riskChoice = -1;
    player.type = PlayerType::Human;
    player.cpuDifficulty = CpuDifficulty::Normal;
    return player;
}

SerializedDeckState& deckStateForSlot(LoadedGameState& data, DeckSlot slot) {
    switch (slot) {
        case DECK_ACTION:
            return data.actionDeck;
        case DECK_COLLEGE_CAREER:
            return data.collegeCareerDeck;
        case DECK_CAREER:
            return data.careerDeck;
        case DECK_HOUSE:
            return data.houseDeck;
        case DECK_INVEST:
            return data.investDeck;
        case DECK_PET:
        default:
            return data.petDeck;
    }
}

void writeDeckState(std::ofstream& out,
                    DeckSlot slot,
                    const SerializedDeckState& state) {
    const std::string slotName = deckSlotName(slot);

    for (std::size_t i = 0; i < state.drawIds.size(); ++i) {
        out << "DECK\t" << slotName << "\tDRAW\t"
            << escapeField(state.drawIds[i]) << "\n";
    }

    for (std::size_t i = 0; i < state.discardIds.size(); ++i) {
        out << "DECK\t" << slotName << "\tDISCARD\t"
            << escapeField(state.discardIds[i]) << "\n";
    }
}

void writePlayer(std::ofstream& out, const Player& player, int index) {
    out << "PLAYER\t" << index << "\tNAME\t" << escapeField(player.name) << "\n";
    out << "PLAYER\t" << index << "\tTYPE\t" << playerTypeLabel(player.type) << "\n";
    out << "PLAYER\t" << index << "\tCPU_DIFFICULTY\t" << cpuDifficultyLabel(player.cpuDifficulty) << "\n";
    out << "PLAYER\t" << index << "\tTOKEN\t" << escapeField(std::string(1, player.token)) << "\n";
    out << "PLAYER\t" << index << "\tTILE\t" << player.tile << "\n";
    out << "PLAYER\t" << index << "\tCASH\t" << player.cash << "\n";
    out << "PLAYER\t" << index << "\tJOB\t" << escapeField(player.job) << "\n";
    out << "PLAYER\t" << index << "\tSALARY\t" << player.salary << "\n";
    out << "PLAYER\t" << index << "\tMARRIED\t" << boolText(player.married) << "\n";
    out << "PLAYER\t" << index << "\tKIDS\t" << player.kids << "\n";
    out << "PLAYER\t" << index << "\tCOLLEGE_GRADUATE\t" << boolText(player.collegeGraduate) << "\n";
    out << "PLAYER\t" << index << "\tUSED_NIGHT_SCHOOL\t" << boolText(player.usedNightSchool) << "\n";
    out << "PLAYER\t" << index << "\tHAS_HOUSE\t" << boolText(player.hasHouse) << "\n";
    out << "PLAYER\t" << index << "\tHOUSE_NAME\t" << escapeField(player.houseName) << "\n";
    out << "PLAYER\t" << index << "\tHOUSE_VALUE\t" << player.houseValue << "\n";
    out << "PLAYER\t" << index << "\tLOANS\t" << player.loans << "\n";
    out << "PLAYER\t" << index << "\tINVEST_NUMBER\t" << player.investedNumber << "\n";
    out << "PLAYER\t" << index << "\tINVEST_PAYOUT\t" << player.investPayout << "\n";
    out << "PLAYER\t" << index << "\tSPIN_TO_WIN\t" << player.spinToWinTokens << "\n";
    out << "PLAYER\t" << index << "\tRETIREMENT_PLACE\t" << player.retirementPlace << "\n";
    out << "PLAYER\t" << index << "\tRETIREMENT_BONUS\t" << player.retirementBonus << "\n";
    out << "PLAYER\t" << index << "\tFINAL_HOUSE_SALE\t" << player.finalHouseSaleValue << "\n";
    out << "PLAYER\t" << index << "\tRETIREMENT_HOME\t" << escapeField(player.retirementHome) << "\n";
    out << "PLAYER\t" << index << "\tRETIRED\t" << boolText(player.retired) << "\n";
    out << "PLAYER\t" << index << "\tSTART_CHOICE\t" << player.startChoice << "\n";
    out << "PLAYER\t" << index << "\tFAMILY_CHOICE\t" << player.familyChoice << "\n";
    out << "PLAYER\t" << index << "\tRISK_CHOICE\t" << player.riskChoice << "\n";

    for (std::size_t i = 0; i < player.actionCards.size(); ++i) {
        out << "PLAYER\t" << index << "\tACTION_CARD\t"
            << escapeField(player.actionCards[i]) << "\n";
    }

    for (std::size_t i = 0; i < player.petCards.size(); ++i) {
        out << "PLAYER\t" << index << "\tPET_CARD\t"
            << escapeField(player.petCards[i]) << "\n";
    }
}

void writeRules(std::ofstream& out, const RuleSet& rules) {
    out << "RULES\tEDITION\t" << escapeField(rules.editionName) << "\n";
    out << "RULES\tTOGGLE\tPETS\t" << boolText(rules.toggles.petsEnabled) << "\n";
    out << "RULES\tTOGGLE\tINVESTMENT\t" << boolText(rules.toggles.investmentEnabled) << "\n";
    out << "RULES\tTOGGLE\tSPIN_TO_WIN\t" << boolText(rules.toggles.spinToWinEnabled) << "\n";
    out << "RULES\tTOGGLE\tELECTRONIC_BANKING\t" << boolText(rules.toggles.electronicBankingEnabled) << "\n";
    out << "RULES\tTOGGLE\tFAMILY_PATH\t" << boolText(rules.toggles.familyPathEnabled) << "\n";
    out << "RULES\tTOGGLE\tRISKY_ROUTE\t" << boolText(rules.toggles.riskyRouteEnabled) << "\n";
    out << "RULES\tTOGGLE\tNIGHT_SCHOOL\t" << boolText(rules.toggles.nightSchoolEnabled) << "\n";
    out << "RULES\tTOGGLE\tTUTORIAL\t" << boolText(rules.toggles.tutorialEnabled) << "\n";
    out << "RULES\tTOGGLE\tHOUSE_SALE_SPIN\t" << boolText(rules.toggles.houseSaleSpinEnabled) << "\n";
    out << "RULES\tTOGGLE\tRETIREMENT_BONUSES\t" << boolText(rules.toggles.retirementBonusesEnabled) << "\n";
    out << "RULES\tCOMPONENT\tACTION\t" << rules.components.actionCards << "\n";
    out << "RULES\tCOMPONENT\tCOLLEGE_CAREER\t" << rules.components.collegeCareerCards << "\n";
    out << "RULES\tCOMPONENT\tCAREER\t" << rules.components.careerCards << "\n";
    out << "RULES\tCOMPONENT\tHOUSE\t" << rules.components.houseCards << "\n";
    out << "RULES\tCOMPONENT\tINVEST\t" << rules.components.investCards << "\n";
    out << "RULES\tCOMPONENT\tPET\t" << rules.components.petCards << "\n";
    out << "RULES\tCOMPONENT\tSPIN_TO_WIN\t" << rules.components.spinToWinTokens << "\n";
    out << "RULES\tLOAN_UNIT\t" << rules.loanUnit << "\n";
    out << "RULES\tLOAN_REPAYMENT\t" << rules.loanRepaymentCost << "\n";
    out << "RULES\tINVEST_PAYOUT\t" << rules.investmentMatchPayout << "\n";
    out << "RULES\tSPIN_PRIZE\t" << rules.spinToWinPrize << "\n";
}

bool parsePlayerField(Player& player,
                      const std::string& field,
                      const std::string& value,
                      std::string& error) {
    if (field == "NAME") {
        player.name = value;
        return true;
    }
    if (field == "TOKEN") {
        player.token = value.empty() ? 'A' : value[0];
        return true;
    }
    if (field == "TYPE") {
        player.type = playerTypeFromText(value);
        return true;
    }
    if (field == "CPU_DIFFICULTY") {
        player.cpuDifficulty = cpuDifficultyFromText(value);
        return true;
    }
    if (field == "JOB") {
        player.job = value;
        return true;
    }
    if (field == "HOUSE_NAME") {
        player.houseName = value;
        return true;
    }
    if (field == "RETIREMENT_HOME") {
        player.retirementHome = value;
        return true;
    }
    if (field == "ACTION_CARD") {
        player.actionCards.push_back(value);
        return true;
    }
    if (field == "PET_CARD") {
        player.petCards.push_back(value);
        return true;
    }

    if (field == "TILE") return parseInt(value, player.tile);
    if (field == "CASH") return parseInt(value, player.cash);
    if (field == "SALARY") return parseInt(value, player.salary);
    if (field == "KIDS") return parseInt(value, player.kids);
    if (field == "HOUSE_VALUE") return parseInt(value, player.houseValue);
    if (field == "LOANS") return parseInt(value, player.loans);
    if (field == "INVEST_NUMBER") return parseInt(value, player.investedNumber);
    if (field == "INVEST_PAYOUT") return parseInt(value, player.investPayout);
    if (field == "SPIN_TO_WIN") return parseInt(value, player.spinToWinTokens);
    if (field == "RETIREMENT_PLACE") return parseInt(value, player.retirementPlace);
    if (field == "RETIREMENT_BONUS") return parseInt(value, player.retirementBonus);
    if (field == "FINAL_HOUSE_SALE") return parseInt(value, player.finalHouseSaleValue);
    if (field == "START_CHOICE") return parseInt(value, player.startChoice);
    if (field == "FAMILY_CHOICE") return parseInt(value, player.familyChoice);
    if (field == "RISK_CHOICE") return parseInt(value, player.riskChoice);
    if (field == "MARRIED") return parseBool(value, player.married);
    if (field == "COLLEGE_GRADUATE") return parseBool(value, player.collegeGraduate);
    if (field == "USED_NIGHT_SCHOOL") return parseBool(value, player.usedNightSchool);
    if (field == "HAS_HOUSE") return parseBool(value, player.hasHouse);
    if (field == "RETIRED") return parseBool(value, player.retired);

    error = "Unknown player field: " + field;
    return false;
}

bool parseRuleField(RuleSet& rules,
                    const std::vector<std::string>& parts,
                    std::string& error) {
    if (parts.size() < 3) {
        error = "Incomplete rules line.";
        return false;
    }

    if (parts[1] == "EDITION") {
        rules.editionName = parts[2];
        return true;
    }

    const std::string& category = parts[1];
    if (category == "LOAN_UNIT") return parseInt(parts[2], rules.loanUnit);
    if (category == "LOAN_REPAYMENT") return parseInt(parts[2], rules.loanRepaymentCost);
    if (category == "INVEST_PAYOUT") return parseInt(parts[2], rules.investmentMatchPayout);
    if (category == "SPIN_PRIZE") return parseInt(parts[2], rules.spinToWinPrize);

    if (parts.size() < 4) {
        error = "Incomplete rules line.";
        return false;
    }

    const std::string& name = parts[2];
    const std::string& value = parts[3];

    if (category == "TOGGLE") {
        if (name == "PETS") return parseBool(value, rules.toggles.petsEnabled);
        if (name == "INVESTMENT") return parseBool(value, rules.toggles.investmentEnabled);
        if (name == "SPIN_TO_WIN") return parseBool(value, rules.toggles.spinToWinEnabled);
        if (name == "ELECTRONIC_BANKING") return parseBool(value, rules.toggles.electronicBankingEnabled);
        if (name == "FAMILY_PATH") return parseBool(value, rules.toggles.familyPathEnabled);
        if (name == "RISKY_ROUTE") return parseBool(value, rules.toggles.riskyRouteEnabled);
        if (name == "NIGHT_SCHOOL") return parseBool(value, rules.toggles.nightSchoolEnabled);
        if (name == "TUTORIAL") return parseBool(value, rules.toggles.tutorialEnabled);
        if (name == "HOUSE_SALE_SPIN") return parseBool(value, rules.toggles.houseSaleSpinEnabled);
        if (name == "RETIREMENT_BONUSES") return parseBool(value, rules.toggles.retirementBonusesEnabled);
    }

    if (category == "COMPONENT") {
        if (name == "ACTION") return parseInt(value, rules.components.actionCards);
        if (name == "COLLEGE_CAREER") return parseInt(value, rules.components.collegeCareerCards);
        if (name == "CAREER") return parseInt(value, rules.components.careerCards);
        if (name == "HOUSE") return parseInt(value, rules.components.houseCards);
        if (name == "INVEST") return parseInt(value, rules.components.investCards);
        if (name == "PET") return parseInt(value, rules.components.petCards);
        if (name == "SPIN_TO_WIN") return parseInt(value, rules.components.spinToWinTokens);
    }

    error = "Unknown rules field: " + category;
    return false;
}
}

bool SaveManager::saveExists(const std::string& filename) const {
    const fs::path path(resolvePath(filename));
    std::error_code errorCode;
    return fs::exists(path, errorCode) && fs::is_regular_file(path, errorCode);
}

bool SaveManager::ensureSaveDirectory(std::string& error) const {
    const fs::path directory = saveDirectoryPath();
    std::error_code errorCode;

    if (fs::exists(directory, errorCode)) {
        if (errorCode) {
            error = "Could not inspect saves folder.";
            return false;
        }
        if (!fs::is_directory(directory, errorCode)) {
            error = "A non-folder item is blocking saves/.";
            return false;
        }
        return true;
    }

    if (!fs::create_directories(directory, errorCode) && errorCode) {
        error = "Could not create saves/ folder.";
        return false;
    }
    return true;
}

std::vector<SaveFileInfo> SaveManager::listSaveFiles(std::string& error) const {
    std::vector<SaveFileInfo> files;
    const fs::path directory = saveDirectoryPath();
    std::error_code errorCode;

    if (!fs::exists(directory, errorCode)) {
        return files;
    }
    if (errorCode) {
        error = "Could not inspect saves/ folder.";
        return files;
    }
    if (!fs::is_directory(directory, errorCode)) {
        error = "saves/ exists but is not a folder.";
        return files;
    }

    for (fs::directory_iterator it(directory, errorCode), end; !errorCode && it != end; it.increment(errorCode)) {
        const fs::directory_entry& entry = *it;
        if (!entry.is_regular_file(errorCode) || errorCode) {
            continue;
        }

        const fs::path path = entry.path();
        if (path.extension() != ".sav") {
            continue;
        }

        SaveFileInfo info;
        info.saveVersion = 0;
        info.filename = path.filename().string();
        info.path = path.string();
        info.gameId.clear();
        info.assignedFilename.clear();
        info.createdText = "unknown";
        info.sizeBytes = entry.file_size(errorCode);
        if (errorCode) {
            info.sizeBytes = 0;
            errorCode.clear();
        }

        const fs::file_time_type modified = entry.last_write_time(errorCode);
        if (errorCode) {
            info.createdTime = 0;
            info.modifiedTime = 0;
            info.modifiedText = "unknown";
            info.lastSavedText = "unknown";
            info.lastSavedTime = 0;
            errorCode.clear();
        } else {
            info.createdTime = 0;
            info.modifiedTime = fileTimeToTimeT(modified);
            info.modifiedText = formatTimestamp(info.modifiedTime, "%Y-%m-%d %H:%M:%S");
            info.lastSavedText = info.modifiedText;
            info.lastSavedTime = info.modifiedTime;
        }
        info.metadataValid = false;
        info.duplicateGameId = false;
        info.assignedTarget = true;
        readSaveFileMetadata(path, info);
        files.push_back(info);
    }

    if (errorCode) {
        error = "Could not read one or more save files.";
        files.clear();
        return files;
    }

    std::map<std::string, int> gameIdCounts;
    for (std::size_t i = 0; i < files.size(); ++i) {
        if (!files[i].gameId.empty()) {
            ++gameIdCounts[files[i].gameId];
        }
    }
    for (std::size_t i = 0; i < files.size(); ++i) {
        if (!files[i].gameId.empty() && gameIdCounts[files[i].gameId] > 1) {
            files[i].duplicateGameId = true;
        }
    }

    std::sort(files.begin(), files.end(), [](const SaveFileInfo& left, const SaveFileInfo& right) {
        const std::time_t leftNewest = left.lastSavedTime > 0 ? left.lastSavedTime : left.modifiedTime;
        const std::time_t rightNewest = right.lastSavedTime > 0 ? right.lastSavedTime : right.modifiedTime;
        if (leftNewest != rightNewest) {
            return leftNewest > rightNewest;
        }
        return left.filename < right.filename;
    });
    return files;
}

std::string SaveManager::defaultSaveFilename() const {
    const std::time_t now = std::time(0);
    return formatTimestamp(now, "%Y-%m-%d_%H-%M-%S.sav");
}

std::string SaveManager::generateGameId() const {
    static std::uint64_t counter = 0;
    const std::uint64_t timePart = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    std::random_device randomDevice;
    const std::uint64_t randomPart =
        (static_cast<std::uint64_t>(randomDevice()) << 32) ^
        static_cast<std::uint64_t>(randomDevice());

    ++counter;
    std::ostringstream out;
    out << "G" << std::hex << std::uppercase
        << timePart << "-" << randomPart << "-" << counter;
    return out.str();
}

std::string SaveManager::normalizeFilename(const std::string& filename) const {
    std::string trimmed = filename;
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed[0]))) {
        trimmed.erase(trimmed.begin());
    }
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed[trimmed.size() - 1]))) {
        trimmed.erase(trimmed.end() - 1);
    }

    if (trimmed.empty()) {
        return defaultSaveFilename();
    }

    fs::path requested(trimmed);
    if (requested.has_filename()) {
        trimmed = requested.filename().string();
    } else {
        return defaultSaveFilename();
    }
    if (trimmed.empty()) {
        return defaultSaveFilename();
    }
    return ensureSavExtension(trimmed);
}

std::string SaveManager::resolvePath(const std::string& filename) const {
    return (saveDirectoryPath() / normalizeFilename(filename)).string();
}

bool SaveManager::archiveDuplicateSaves(const std::string& gameId,
                                        const std::string& keepFilename,
                                        int& archivedCount,
                                        std::string& error) const {
    archivedCount = 0;
    error.clear();
    if (gameId.empty()) {
        return true;
    }
    if (!ensureSaveDirectory(error)) {
        return false;
    }

    const std::string keepNormalized = normalizeFilename(keepFilename);
    const fs::path directory = saveDirectoryPath();
    std::error_code errorCode;
    for (fs::directory_iterator it(directory, errorCode), end; !errorCode && it != end; it.increment(errorCode)) {
        const fs::directory_entry& entry = *it;
        if (!entry.is_regular_file(errorCode) || errorCode) {
            continue;
        }

        const fs::path sourcePath = entry.path();
        if (sourcePath.extension() != ".sav") {
            continue;
        }
        if (sourcePath.filename().string() == keepNormalized) {
            continue;
        }

        SaveFileInfo info;
        info.saveVersion = 0;
        info.filename = sourcePath.filename().string();
        info.path = sourcePath.string();
        info.gameId.clear();
        info.assignedFilename.clear();
        info.createdText = "unknown";
        info.modifiedText = "unknown";
        info.lastSavedText = "unknown";
        info.sizeBytes = 0;
        info.createdTime = 0;
        info.modifiedTime = 0;
        info.lastSavedTime = 0;
        info.metadataValid = false;
        info.duplicateGameId = false;
        info.assignedTarget = false;

        if (!readSaveFileMetadata(sourcePath, info) || info.gameId != gameId) {
            continue;
        }

        const fs::path archivePath = makeArchivePath(sourcePath);
        fs::rename(sourcePath, archivePath, errorCode);
        if (errorCode) {
            error = "Saved, but could not archive duplicate file " + sourcePath.filename().string() + ".";
            return false;
        }
        ++archivedCount;
    }

    if (errorCode) {
        error = "Saved, but duplicate cleanup could not finish.";
        return false;
    }
    return true;
}

bool SaveManager::saveGame(const Game& game,
                           const std::string& filename,
                           std::string& error) const {
    if (!ensureSaveDirectory(error)) {
        return false;
    }

    const std::string resolvedPath = resolvePath(filename);
    std::ofstream out(resolvedPath.c_str(), std::ios::out | std::ios::trunc);
    if (!out) {
        error = "Could not open file for writing.";
        return false;
    }

    out << "GOLDRUSH_SAVE\t" << SAVE_VERSION << "\n";
    out << "GAME\tID\t" << escapeField(game.gameId) << "\n";
    out << "GAME\tCREATED_AT\t" << static_cast<long long>(game.createdTime) << "\n";
    out << "GAME\tLAST_SAVED_AT\t" << static_cast<long long>(game.lastSavedTime) << "\n";
    out << "GAME\tASSIGNED_FILE\t" << escapeField(game.assignedSaveFilename) << "\n";
    out << "GAME\tCURRENT_PLAYER\t" << game.currentPlayerIndex << "\n";
    out << "GAME\tTURN_COUNTER\t" << game.turnCounter << "\n";
    out << "GAME\tRETIRED_COUNT\t" << game.retiredCount << "\n";
    out << "GAME\tRNG_FIXED\t" << boolText(game.rng.usesFixedSeed()) << "\n";
    out << "GAME\tRNG_SEED\t" << game.rng.seed() << "\n";
    out << "GAME\tRNG_STATE\t" << escapeField(game.rng.serializeState()) << "\n";

    writeRules(out, game.rules);

    out << "PLAYERS\tCOUNT\t" << game.players.size() << "\n";
    for (std::size_t i = 0; i < game.players.size(); ++i) {
        writePlayer(out, game.players[i], static_cast<int>(i));
    }

    const std::vector<std::string> historyEntries = game.history.recent();
    for (std::size_t i = 0; i < historyEntries.size(); ++i) {
        out << "HISTORY\tENTRY\t" << escapeField(historyEntries[i]) << "\n";
    }

    writeDeckState(out, DECK_ACTION, game.decks.deckState(DECK_ACTION));
    writeDeckState(out, DECK_COLLEGE_CAREER, game.decks.deckState(DECK_COLLEGE_CAREER));
    writeDeckState(out, DECK_CAREER, game.decks.deckState(DECK_CAREER));
    writeDeckState(out, DECK_HOUSE, game.decks.deckState(DECK_HOUSE));
    writeDeckState(out, DECK_INVEST, game.decks.deckState(DECK_INVEST));
    writeDeckState(out, DECK_PET, game.decks.deckState(DECK_PET));
    out << "END\tSAVE\n";

    if (!out.good()) {
        error = "Failed while writing save data.";
        return false;
    }
    return true;
}

bool SaveManager::loadGame(Game& game,
                           const std::string& filename,
                           std::string& error) const {
    const std::string resolvedPath = resolvePath(filename);
    std::ifstream in(resolvedPath.c_str());
    if (!in) {
        error = "Save file not found.";
        return false;
    }

    std::string line;
    if (!std::getline(in, line)) {
        error = "Save file is empty.";
        return false;
    }

    std::vector<std::string> headerParts = splitTabbed(line);
    if (headerParts.size() != 2 || headerParts[0] != "GOLDRUSH_SAVE") {
        error = "Invalid save header.";
        return false;
    }

    int version = 0;
    if (!parseInt(headerParts[1], version)) {
        error = "Invalid save version.";
        return false;
    }
    if (version < 1 || version > SAVE_VERSION) {
        error = "Unsupported save version.";
        return false;
    }

    LoadedGameState data;
    data.rules = makeNormalRules();
    data.rngFixedSeed = false;
    data.rngSeedValue = 0;
    data.currentPlayerIndex = 0;
    data.turnCounter = 0;
    data.retiredCount = 0;
    data.gameId.clear();
    data.assignedFilename.clear();
    data.createdTime = 0;
    data.lastSavedTime = 0;

    std::time_t fileModifiedTime = std::time(0);
    std::error_code fileTimeError;
    const fs::file_time_type fileWriteTime = fs::last_write_time(fs::path(resolvedPath), fileTimeError);
    if (!fileTimeError) {
        fileModifiedTime = fileTimeToTimeT(fileWriteTime);
    }

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        // Save data is stored as tab-separated records. Every field is escaped on
        // write, so loading always begins by splitting the raw line and decoding
        // each field before dispatching on the record prefix.
        std::vector<std::string> parts = splitTabbed(line);
        for (std::size_t i = 0; i < parts.size(); ++i) {
            std::string decoded;
            if (!unescapeField(parts[i], decoded)) {
                error = "Corrupted escape sequence in save file.";
                return false;
            }
            parts[i] = decoded;
        }

        if (parts.empty()) {
            continue;
        }

        if (parts[0] == "END") {
            break;
        }

        if (parts[0] == "GAME") {
            if (parts.size() < 3) {
                error = "Incomplete game state line.";
                return false;
            }

            if (parts[1] == "ID") {
                data.gameId = parts[2];
            } else if (parts[1] == "CREATED_AT") {
                if (!parseTimeValue(parts[2], data.createdTime)) {
                    error = "Invalid created time.";
                    return false;
                }
            } else if (parts[1] == "LAST_SAVED_AT") {
                if (!parseTimeValue(parts[2], data.lastSavedTime)) {
                    error = "Invalid last-saved time.";
                    return false;
                }
            } else if (parts[1] == "ASSIGNED_FILE") {
                data.assignedFilename = parts[2];
            } else if (parts[1] == "CURRENT_PLAYER") {
                if (!parseInt(parts[2], data.currentPlayerIndex)) {
                    error = "Invalid current player index.";
                    return false;
                }
            } else if (parts[1] == "TURN_COUNTER") {
                if (!parseInt(parts[2], data.turnCounter)) {
                    error = "Invalid turn counter.";
                    return false;
                }
            } else if (parts[1] == "RETIRED_COUNT") {
                if (!parseInt(parts[2], data.retiredCount)) {
                    error = "Invalid retired count.";
                    return false;
                }
            } else if (parts[1] == "RNG_FIXED") {
                if (!parseBool(parts[2], data.rngFixedSeed)) {
                    error = "Invalid RNG fixed-seed flag.";
                    return false;
                }
            } else if (parts[1] == "RNG_SEED") {
                if (!parseUInt(parts[2], data.rngSeedValue)) {
                    error = "Invalid RNG seed value.";
                    return false;
                }
            } else if (parts[1] == "RNG_STATE") {
                data.rngState = parts[2];
            } else {
                error = "Unknown game state field: " + parts[1];
                return false;
            }
            continue;
        }

        if (parts[0] == "RULES") {
            if (!parseRuleField(data.rules, parts, error)) {
                if (error.empty()) {
                    error = "Invalid rules entry.";
                }
                return false;
            }
            continue;
        }

        if (parts[0] == "PLAYERS") {
            if (parts.size() != 3 || parts[1] != "COUNT") {
                error = "Invalid player count line.";
                return false;
            }

            int playerCount = 0;
            if (!parseInt(parts[2], playerCount) || playerCount < 1) {
                error = "Invalid player count.";
                return false;
            }

            data.players.clear();
            data.players.reserve(static_cast<std::size_t>(playerCount));
            for (int i = 0; i < playerCount; ++i) {
                data.players.push_back(makeDefaultPlayer(i));
            }
            continue;
        }

        if (parts[0] == "PLAYER") {
            if (parts.size() != 4) {
                error = "Invalid player entry.";
                return false;
            }

            int playerIndex = 0;
            if (!parseInt(parts[1], playerIndex) ||
                playerIndex < 0 ||
                playerIndex >= static_cast<int>(data.players.size())) {
                error = "Player entry index is out of range.";
                return false;
            }

            if (!parsePlayerField(data.players[static_cast<std::size_t>(playerIndex)],
                                  parts[2],
                                  parts[3],
                                  error)) {
                if (error.empty()) {
                    error = "Invalid player field.";
                }
                return false;
            }
            continue;
        }

        if (parts[0] == "HISTORY") {
            if (parts.size() != 3 || parts[1] != "ENTRY") {
                error = "Invalid history entry.";
                return false;
            }
            data.historyEntries.push_back(parts[2]);
            continue;
        }

        if (parts[0] == "DECK") {
            if (parts.size() != 4) {
                error = "Invalid deck entry.";
                return false;
            }

            DeckSlot slot;
            if (!parseDeckSlot(parts[1], slot)) {
                error = "Unknown deck slot.";
                return false;
            }

            SerializedDeckState& state = deckStateForSlot(data, slot);
            if (parts[2] == "DRAW") {
                state.drawIds.push_back(parts[3]);
            } else if (parts[2] == "DISCARD") {
                state.discardIds.push_back(parts[3]);
            } else {
                error = "Unknown deck pile.";
                return false;
            }
            continue;
        }

        error = "Unknown save record type: " + parts[0];
        return false;
    }

    if (data.players.empty()) {
        error = "Save file does not contain any players.";
        return false;
    }
    if (data.currentPlayerIndex < 0 ||
        data.currentPlayerIndex >= static_cast<int>(data.players.size())) {
        error = "Saved current player index is out of range.";
        return false;
    }
    if (data.gameId.empty()) {
        data.gameId = generateGameId();
    }
    if (data.assignedFilename.empty()) {
        data.assignedFilename = fs::path(resolvedPath).filename().string();
    }
    data.assignedFilename = normalizeFilename(data.assignedFilename);
    if (data.createdTime <= 0) {
        data.createdTime = fileModifiedTime;
    }
    if (data.lastSavedTime <= 0) {
        data.lastSavedTime = fileModifiedTime;
    }
    if (!game.rng.restoreState(data.rngState, data.rngFixedSeed, data.rngSeedValue)) {
        error = "Could not restore RNG state.";
        return false;
    }

    game.rules = data.rules;
    game.bank.configure(game.rules);
    game.players = data.players;
    game.currentPlayerIndex = data.currentPlayerIndex;
    game.turnCounter = data.turnCounter;
    game.retiredCount = data.retiredCount;
    game.gameId = data.gameId;
    game.assignedSaveFilename = data.assignedFilename;
    game.createdTime = data.createdTime;
    game.lastSavedTime = data.lastSavedTime;
    game.decks.reset(game.rules, false);

    // The deck order and RNG state are restored after the high-level game data
    // so future draws and rolls continue from the exact same timeline.
    if (!game.decks.restoreDeckState(DECK_ACTION, data.actionDeck, error)) return false;
    if (!game.decks.restoreDeckState(DECK_COLLEGE_CAREER, data.collegeCareerDeck, error)) return false;
    if (!game.decks.restoreDeckState(DECK_CAREER, data.careerDeck, error)) return false;
    if (!game.decks.restoreDeckState(DECK_HOUSE, data.houseDeck, error)) return false;
    if (!game.decks.restoreDeckState(DECK_INVEST, data.investDeck, error)) return false;
    if (!game.decks.restoreDeckState(DECK_PET, data.petDeck, error)) return false;

    game.history.restore(data.historyEntries);
    return true;
}
