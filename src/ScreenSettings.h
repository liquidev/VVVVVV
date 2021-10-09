#ifndef SCREENSETTINGS_H
#define SCREENSETTINGS_H

enum ScreenScaling {
    ssOneToOne,
    ssFourToThree,
    ssFullscreen,
    ss_Last,
};

struct ScreenSettings
{
    ScreenSettings(void);

    int windowWidth;
    int windowHeight;
    bool fullscreen;
    bool useVsync;
    ScreenScaling scalingMode;
    bool linearFilter;
    bool badSignal;
};

#endif /* SCREENSETTINGS_H */
