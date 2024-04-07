/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Debug.hpp"
#include "Error.hpp"
#include "Emulation.hpp"

#include "SaveState.hpp"
#include "m64p/Api.hpp"

#include <fstream>

//
// Local Variables
//

static CoreCompareMode l_CoreCompareMode;

static std::fstream l_RecordingFileStream;

//
// Internal Functions
//

#include <iostream>
static void core_compare_check(uint32_t op)
{
}

static void core_compare_sync(int length, void* value)
{

	if (l_CoreCompareMode == CoreCompareMode::Replay)
    {
    	l_RecordingFileStream.read((char*)value, length);

    	if (l_RecordingFileStream.gcount() != length)
    	{
    		CoreCompareStop();
    	}
	}
	else
    {
    	l_RecordingFileStream.write((char*)value, length);
    }
}

//
// Exported Functions
//

bool CoreCompareStart(CoreCompareMode mode)
{
	std::string error;
	m64p_error  ret;

	l_CoreCompareMode = mode;

	if (l_CoreCompareMode == CoreCompareMode::Replay)
	{
		if (!CoreLoadSaveState("rmg_recording.state"))
		{
			return false;
		}
	}
	else
	{
		if (!CoreSaveState("rmg_recording.state"))
		{
			return false;
		}
	}

	ret = m64p::Debug.SetCoreCompare(core_compare_check, core_compare_sync);
	if (ret != M64ERR_SUCCESS)
	{
		error = "CoreCompareInit: m64p::Debug.SetCoreCompare() Failed: ";
		error += m64p::Core.ErrorMessage(ret);
		CoreSetError(error);
		return false;
	}

	if (l_CoreCompareMode == CoreCompareMode::Replay)
	{
		l_RecordingFileStream.open("rmg_recording", std::ios_base::binary | std::ios_base::in);
	}
	else
	{
		l_RecordingFileStream.open("rmg_recording", std::ios_base::binary | std::ios_base::out);
	}

	return true;
}

bool CoreCompareStop(void)
{
	std::string error;
	m64p_error  ret;

	ret = m64p::Debug.SetCoreCompare(nullptr, nullptr);
	if (ret != M64ERR_SUCCESS)
	{
		error = "CoreCompareInit: m64p::Debug.SetCoreCompare() Failed: ";
		error += m64p::Core.ErrorMessage(ret);
		CoreSetError(error);
	}

	l_RecordingFileStream.close();

	return ret == M64ERR_SUCCESS;
}
