#include <SDL2/SDL_keycode.h>
#define KEY_DEFINITION
#include "KeyPoll.h"

#include <SDL2/SDL.h>
#include <string.h>
#include <utf8/unchecked.h>

#include "Exit.h"
#include "Game.h"
#include "GlitchrunnerMode.h"
#include "Graphics.h"
#include "Music.h"
#include "Vlogging.h"

#include <pspctrl.h>

int inline KeyPoll::getThreshold(void)
{
    switch (sensitivity)
    {
    case 0:
        return 28000;
    case 1:
        return 16000;
    case 2:
        return 8000;
    case 3:
        return 4000;
    case 4:
        return 2000;
    }

    return 8000;

}

KeyPoll::KeyPoll(void)
{
    xVel = 0;
    yVel = 0;
    // 0..5
    sensitivity = 2;

    keybuffer="";
    leftbutton=0; rightbutton=0; middlebutton=0;
    mx=0; my=0;
    resetWindow = 0;
    pressedbackspace=false;

    linealreadyemptykludge = false;

    isActive = true;
}

void KeyPoll::enabletextentry(void)
{
    keybuffer="";
    SDL_StartTextInput();
}

void KeyPoll::disabletextentry(void)
{
    SDL_StopTextInput();
}

bool KeyPoll::textentry(void)
{
    return SDL_IsTextInputActive() == SDL_TRUE;
}

static int changemousestate(
    int timeout,
    const bool show,
    const bool hide
) {
    int prev;
    int new_;

    if (timeout > 0)
    {
        return --timeout;
    }

    /* If we want to both show and hide at the same time, prioritize showing */
    if (show)
    {
        new_ = SDL_ENABLE;
    }
    else if (hide)
    {
        new_ = SDL_DISABLE;
    }
    else
    {
        return timeout;
    }

    prev = SDL_ShowCursor(SDL_QUERY);

    if (prev == new_)
    {
        return timeout;
    }

    SDL_ShowCursor(new_);

    switch (new_)
    {
    case SDL_DISABLE:
        timeout = 0;
        break;
    case SDL_ENABLE:
        timeout = 30;
        break;
    }

    return timeout;
}

void KeyPoll::Poll(void)
{
    SceCtrlData pad = {0};

    if (sceCtrlPeekBufferPositive(&pad, 1)) {
        buttonmap[SDL_CONTROLLER_BUTTON_A] = (pad.Buttons & PSP_CTRL_CROSS) != 0;
        buttonmap[SDL_CONTROLLER_BUTTON_B] = (pad.Buttons & PSP_CTRL_CIRCLE) != 0;
        buttonmap[SDL_CONTROLLER_BUTTON_X] = (pad.Buttons & PSP_CTRL_SQUARE) != 0;
        buttonmap[SDL_CONTROLLER_BUTTON_Y] = (pad.Buttons & PSP_CTRL_TRIANGLE) != 0;
        buttonmap[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = (pad.Buttons & PSP_CTRL_RIGHT) != 0;
        buttonmap[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = (pad.Buttons & PSP_CTRL_DOWN) != 0;
        buttonmap[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = (pad.Buttons & PSP_CTRL_LEFT) != 0;
        buttonmap[SDL_CONTROLLER_BUTTON_DPAD_UP] = (pad.Buttons & PSP_CTRL_UP) != 0;
    }
}

bool KeyPoll::isDown(SDL_Keycode key)
{
    return keymap[key];
}

bool KeyPoll::isDown(const std::vector<SDL_GameControllerButton> &buttons)
{
    for (size_t i = 0; i < buttons.size(); i += 1)
    {
        if (buttonmap[buttons[i]])
        {
            return true;
        }
    }
    return false;
}

bool KeyPoll::isDown(SDL_GameControllerButton button)
{
    return buttonmap[button];
}

bool KeyPoll::controllerButtonDown(void)
{
    for (
        SDL_GameControllerButton button = SDL_CONTROLLER_BUTTON_A;
        button < SDL_CONTROLLER_BUTTON_DPAD_UP;
        button = (SDL_GameControllerButton) (button + 1)
    ) {
        if (isDown(button))
        {
            return true;
        }
    }
    return false;
}

bool KeyPoll::controllerWantsLeft(bool includeVert)
{
    return (    buttonmap[SDL_CONTROLLER_BUTTON_DPAD_LEFT] ||
            xVel < 0 ||
            (    includeVert &&
                (    buttonmap[SDL_CONTROLLER_BUTTON_DPAD_UP] ||
                    yVel < 0    )    )    );
}

bool KeyPoll::controllerWantsRight(bool includeVert)
{
    return (    buttonmap[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] ||
            xVel > 0 ||
            (    includeVert &&
                (    buttonmap[SDL_CONTROLLER_BUTTON_DPAD_DOWN] ||
                    yVel > 0    )    )    );
}
