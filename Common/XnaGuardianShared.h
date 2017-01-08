/*
MIT License

Copyright (c) 2016 Benjamin "Nefarius" H�glinger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#pragma once

#define XNA_GUARDIAN_DEVICE_PATH    TEXT("\\\\.\\XnaGuardian")

#define XINPUT_MAX_DEVICES          0x04

//
// Custom extensions
// 
#define XINPUT_EXT_TYPE                         0x8001
#define XINPUT_EXT_CODE                         0x801

#define IOCTL_XINPUT_EXT_HIDE_GAMEPAD           CTL_CODE(XINPUT_EXT_TYPE, XINPUT_EXT_CODE + 0x00, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE CTL_CODE(XINPUT_EXT_TYPE, XINPUT_EXT_CODE + 0x01, METHOD_BUFFERED, FILE_WRITE_DATA)

typedef enum _XINPUT_GAMEPAD_OVERRIDES
{
    XINPUT_GAMEPAD_OVERRIDE_DPAD_UP = 1 << 0,
    XINPUT_GAMEPAD_OVERRIDE_DPAD_DOWN = 1 << 1,
    XINPUT_GAMEPAD_OVERRIDE_DPAD_LEFT = 1 << 2,
    XINPUT_GAMEPAD_OVERRIDE_DPAD_RIGHT = 1 << 3,
    XINPUT_GAMEPAD_OVERRIDE_START = 1 << 4,
    XINPUT_GAMEPAD_OVERRIDE_BACK = 1 << 5,
    XINPUT_GAMEPAD_OVERRIDE_LEFT_THUMB = 1 << 6,
    XINPUT_GAMEPAD_OVERRIDE_RIGHT_THUMB = 1 << 7,
    XINPUT_GAMEPAD_OVERRIDE_LEFT_SHOULDER = 1 << 8,
    XINPUT_GAMEPAD_OVERRIDE_RIGHT_SHOULDER = 1 << 9,
    XINPUT_GAMEPAD_OVERRIDE_A = 1 << 10,
    XINPUT_GAMEPAD_OVERRIDE_B = 1 << 11,
    XINPUT_GAMEPAD_OVERRIDE_X = 1 << 12,
    XINPUT_GAMEPAD_OVERRIDE_Y = 1 << 13,
    XINPUT_GAMEPAD_OVERRIDE_LEFT_TRIGGER = 1 << 14,
    XINPUT_GAMEPAD_OVERRIDE_RIGHT_TRIGGER = 1 << 15,
    XINPUT_GAMEPAD_OVERRIDE_LEFT_THUMB_X = 1 << 16,
    XINPUT_GAMEPAD_OVERRIDE_LEFT_THUMB_Y = 1 << 17,
    XINPUT_GAMEPAD_OVERRIDE_RIGHT_THUMB_X = 1 << 18,
    XINPUT_GAMEPAD_OVERRIDE_RIGHT_THUMB_Y = 1 << 19
} XINPUT_GAMEPAD_OVERRIDES, *PXINPUT_GAMEPAD_OVERRIDES;

//
// State of the gamepad (compatible to XINPUT_GAMEPAD)
// 
typedef struct _XINPUT_GAMEPAD_STATE {
    USHORT wButtons;
    BYTE   bLeftTrigger;
    BYTE   bRightTrigger;
    SHORT  sThumbLX;
    SHORT  sThumbLY;
    SHORT  sThumbRX;
    SHORT  sThumbRY;
} XINPUT_GAMEPAD_STATE, *PXINPUT_GAMEPAD_STATE;

//
// Context data for IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE I/O control code
// 
typedef struct _XINPUT_EXT_OVERRIDE_GAMEPAD
{
    IN ULONG Size;

    IN UCHAR UserIndex;

    IN ULONG Overrides;

    IN XINPUT_GAMEPAD_STATE Gamepad;

} XINPUT_EXT_OVERRIDE_GAMEPAD, *PXINPUT_EXT_OVERRIDE_GAMEPAD;

VOID FORCEINLINE XINPUT_EXT_OVERRIDE_GAMEPAD_INIT(
    _Out_ PXINPUT_EXT_OVERRIDE_GAMEPAD OverrideGamepad,
    _In_ UCHAR UserIndex
)
{
    RtlZeroMemory(OverrideGamepad, sizeof(XINPUT_EXT_OVERRIDE_GAMEPAD));

    OverrideGamepad->Size = sizeof(XINPUT_EXT_OVERRIDE_GAMEPAD);
    OverrideGamepad->UserIndex = UserIndex;
}

