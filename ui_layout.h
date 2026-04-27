#pragma once

struct UILayout {
    bool compact;
    int headerHeight;
    int boardWidth;
    int boardHeight;
    int sidePanelWidth;
    int sidePanelHeight;
    int messageHeight;
    int totalWidth;
    int totalHeight;
    int originX;
    int originY;
};

UILayout calculateUILayout(int termHeight = -1, int termWidth = -1);
int minimumGameWidth();
int minimumGameHeight();
