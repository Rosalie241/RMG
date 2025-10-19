#ifndef INPUTPROFILEDB_HPP
#define INPUTPROFILEDB_HPP

#include <QStringList>
#include <QString>
#include <QList>

#include "common.hpp"

struct InputProfileDBInputEntry
{
	N64ControllerButton button;
	InputAxisDirection axis;

	QList<InputType> inputTypes;
	QStringList names;
	QList<int> data;
	QList<int> extraData;
};

struct InputProfileDBEntry
{
	bool valid = false;

	QStringList deviceNames;
	InputDeviceType deviceType;

	QList<InputProfileDBInputEntry> inputEntries;
};

namespace Utilities
{

class InputProfileDB
{
private:
	QList<InputProfileDBEntry> entries;

public:
	// attempts to load DB in memory
	bool Load(void);

	// attempts to find entry using device name and type
	InputProfileDBEntry FindEntry(const InputDevice& device, bool fallback = true);
};

} // namespace Utilities

#endif // INPUTPROFILEDB_HPP
