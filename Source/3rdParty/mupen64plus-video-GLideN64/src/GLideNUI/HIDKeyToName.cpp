#include "HIDKeyToName.h"
#include "keycode/keycode.h"

QString HIDKeyToName(unsigned int key)
{
	QString returnValue;

	switch (key)
	{
		default:
			returnValue = "Unknown";
			break;
		case KEY_A:
			returnValue = "A";
			break;
		case KEY_B:
			returnValue = "B";
			break;
		case KEY_C:
			returnValue = "C";
			break;
		case KEY_D:
			returnValue = "D";
			break;
		case KEY_E:
			returnValue = "E";
			break;
		case KEY_F:
			returnValue = "F";
			break;
		case KEY_G:
			returnValue = "G";
			break;
		case KEY_H:
			returnValue = "H";
			break;
		case KEY_I:
			returnValue = "I";
			break;
		case KEY_J:
			returnValue = "J";
			break;
		case KEY_K:
			returnValue = "K";
			break;
		case KEY_L:
			returnValue = "L";
			break;
		case KEY_M:
			returnValue = "M";
			break;
		case KEY_N:
			returnValue = "N";
			break;
		case KEY_O:
			returnValue = "O";
			break;
		case KEY_P:
			returnValue = "P";
			break;
		case KEY_Q:
			returnValue = "Q";
			break;
		case KEY_R:
			returnValue = "R";
			break;
		case KEY_S:
			returnValue = "S";
			break;
		case KEY_T:
			returnValue = "T";
			break;
		case KEY_U:
			returnValue = "U";
			break;
		case KEY_V:
			returnValue = "V";
			break;
		case KEY_W:
			returnValue = "W";
			break;
		case KEY_X:
			returnValue = "X";
			break;
		case KEY_Y:
			returnValue = "Y";
			break;
		case KEY_Z:
			returnValue = "Z";
			break;
		case KEY_1:
			returnValue = "1";
			break;
		case KEY_2:
			returnValue = "2";
			break;
		case KEY_3:
			returnValue = "3";
			break;
		case KEY_4:
			returnValue = "4";
			break;
		case KEY_5:
			returnValue = "5";
			break;
		case KEY_6:
			returnValue = "6";
			break;
		case KEY_7:
			returnValue = "7";
			break;
		case KEY_8:
			returnValue = "8";
			break;
		case KEY_9:
			returnValue = "9";
			break;
		case KEY_0:
			returnValue = "0";
			break;
		case KEY_Escape:
			returnValue = "Escape";
			break;
		case KEY_Delete:
			returnValue = "Delete";
			break;
		case KEY_Tab:
			returnValue = "Tab";
			break;
		case KEY_Space:
			returnValue = "Space";
			break;
		case KEY_Minus:
			returnValue = "Minus";
			break;
		case KEY_Equals:
			returnValue = "Equals";
			break;
		case KEY_LeftBracket:
			returnValue = "{";
			break;
		case KEY_RightBracket:
			returnValue = "}";
			break;
		case KEY_Backslash:
			returnValue = "\\";
			break;
		case KEY_Semicolon:
			returnValue = ";";
			break;
		case KEY_Quote:
			returnValue = "\"";
			break;
		case KEY_Grave:
			returnValue = "Grave";
			break;
		case KEY_Comma:
			returnValue = ",";
			break;
		case KEY_Period:
			returnValue = ".";
			break;
		case KEY_Slash:
			returnValue = "/";
			break;
		case KEY_CapsLock:
			returnValue = "Capslock";
			break;
		case KEY_F1:
			returnValue = "F1";
			break;
		case KEY_F2:
			returnValue = "F2";
			break;
		case KEY_F3:
			returnValue = "F3";
			break;
		case KEY_F4:
			returnValue = "F4";
			break;
		case KEY_F5:
			returnValue = "F5";
			break;
		case KEY_F6:
			returnValue = "F6";
			break;
		case KEY_F7:
			returnValue = "F7";
			break;
		case KEY_F8:
			returnValue = "F8";
			break;
		case KEY_F9:
			returnValue = "F9";
			break;
		case KEY_F10:
			returnValue = "F10";
			break;
		case KEY_F11:
			returnValue = "F11";
			break;
		case KEY_F12:
			returnValue = "F12";
			break;
		case KEY_PrintScreen:
			returnValue = "Print Screen";
			break;
		case KEY_ScrollLock:
			returnValue = "Scroll Lock";
			break;
		case KEY_Pause:
			returnValue = "Pause";
			break;
		case KEY_Insert:
			returnValue = "Insert";
			break;
		case KEY_Home:
			returnValue = "Home";
			break;
		case KEY_PageUp:
			returnValue = "Page Up";
			break;
		case KEY_DeleteForward:
			returnValue = "Delete Forward";
			break;
		case KEY_End:
			returnValue = "End";
			break;
		case KEY_PageDown:
			returnValue = "Page Down";
			break;
		case KEY_Right:
			returnValue = "Right";
			break;
		case KEY_Left:
			returnValue = "Left";
			break;
		case KEY_Down:
			returnValue = "Down";
			break;
		case KEY_Up:
			returnValue = "Up";
			break;
		case KP_NumLock:
			returnValue = "[Keypad] Numlock";
			break;
		case KP_Divide:
			returnValue = "[Keypad] /";
			break;
		case KP_Multiply:
			returnValue = "[Keypad] *";
			break;
		case KP_Subtract:
			returnValue = "[Keypad] -";
			break;
		case KP_Add:
			returnValue = "[Keypad] +";
			break;
		case KP_Enter:
			returnValue = "[Keypad] Enter";
			break;
		case KP_1:
			returnValue = "[Keypad] 1";
			break;
		case KP_2:
			returnValue = "[Keypad] 2";
			break;
		case KP_3:
			returnValue = "[Keypad] 3";
			break;
		case KP_4:
			returnValue = "[Keypad] 4";
			break;
		case KP_5:
			returnValue = "[Keypad] 5";
			break;
		case KP_6:
			returnValue = "[Keypad] 6";
			break;
		case KP_7:
			returnValue = "[Keypad] 7";
			break;
		case KP_8:
			returnValue = "[Keypad] 8";
			break;
		case KP_9:
			returnValue = "[Keypad] 9";
			break;
		case KP_0:
			returnValue = "[Keypad] 0";
			break;
		case KP_Point:
			returnValue = "[Keypad] .";
			break;
		case KEY_NonUSBackslash:
			returnValue = "\\";
			break;
		case KP_Equals:
			returnValue = "=";
			break;
		case KEY_F13:
			returnValue = "F13";
			break;
		case KEY_F14:
			returnValue = "F14";
			break;
		case KEY_F15:
			returnValue = "F15";
			break;
		case KEY_F16:
			returnValue = "F16";
			break;
		case KEY_F17:
			returnValue = "F17";
			break;
		case KEY_F18:
			returnValue = "F18";
			break;
		case KEY_F19:
			returnValue = "F19";
			break;
		case KEY_F20:
			returnValue = "F20";
			break;
		case KEY_F21:
			returnValue = "F21";
			break;
		case KEY_F22:
			returnValue = "F22";
			break;
		case KEY_F23:
			returnValue = "F23";
			break;
		case KEY_F24:
			returnValue = "F24";
			break;
		case KEY_Help:
			returnValue = "Help";
			break;
		case KEY_Menu:
			returnValue = "Menu";
			break;
		case KEY_Mute:
			returnValue = "Mute";
			break;
		case KEY_SysReq:
			returnValue = "Sys";
			break;
		case KEY_Return:
			returnValue = "[Keypad] Return";
			break;
		case KP_Clear:
			returnValue = "[Keypad] Clear";
			break;
		case KP_Decimal:
			returnValue = "[Keypad] .";
			break;
		case KEY_LeftControl:
			returnValue = "[Keypad] Left";
			break;
		case KEY_LeftShift:
			returnValue = "Left Shift";
			break;
		case KEY_LeftAlt:
			returnValue = "Left Alt";
			break;
		case KEY_LeftGUI:
			returnValue = "Left Super";
			break;
		case KEY_RightControl:
			returnValue = "Right Control";
			break;
		case KEY_RightShift:
			returnValue = "Right Shift";
			break;
		case KEY_RightAlt:
			returnValue = "Right Alt";
			break;
		case KEY_RightGUI:
			returnValue = "Right Super";
			break;
	}

	return returnValue;
}


