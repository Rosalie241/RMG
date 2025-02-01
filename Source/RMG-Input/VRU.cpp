/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "main.hpp"

#define M64P_PLUGIN_PROTOTYPES 1
#include <RMG-Core/m64p/api/m64p_types.h>
#include <RMG-Core/osal/osal_dynlib.hpp>

#include <RMG-Core/Directories.hpp>
#include <RMG-Core/Archive.hpp>
#include <RMG-Core/Error.hpp>

#include <3rdParty/vosk-api/include/vosk_api.h>

#include <QStringDecoder>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QByteArray>
#include <QJsonArray>

#include <SDL.h>

#include "VRUwords.hpp"
#include "VRU.hpp"

//
// Local Enums
//

enum Language
{
    English        = 0,
    JapaneseOrDemo = 1
};

//
// Local Variables
//

// vosk-api library variables
//

typedef VoskModel*      (*ptr_vosk_model_new)(const char *);
typedef void            (*ptr_vosk_model_free)(VoskModel *);
typedef VoskRecognizer* (*ptr_vosk_recognizer_new_grm)(VoskModel *, float, const char *);
typedef void            (*ptr_vosk_recognizer_free)(VoskRecognizer *);
typedef int             (*ptr_vosk_recognizer_accept_waveform)(VoskRecognizer *, const char *, int );
typedef const char *    (*ptr_vosk_recognizer_final_result)(VoskRecognizer *);
typedef void            (*ptr_vosk_set_log_level)(int);
typedef void            (*ptr_vosk_recognizer_set_max_alternatives)(VoskRecognizer *, int);

static ptr_vosk_model_new                       l_vosk_model_new = nullptr;
static ptr_vosk_model_free                      l_vosk_model_free = nullptr;
static ptr_vosk_recognizer_new_grm              l_vosk_recognizer_new_grm = nullptr;
static ptr_vosk_recognizer_free                 l_vosk_recognizer_free = nullptr;
static ptr_vosk_recognizer_accept_waveform      l_vosk_recognizer_accept_waveform = nullptr;
static ptr_vosk_recognizer_final_result         l_vosk_recognizer_final_result = nullptr;
static ptr_vosk_set_log_level                   l_vosk_set_log_level = nullptr;
static ptr_vosk_recognizer_set_max_alternatives l_vosk_recognizer_set_max_alternatives = nullptr;

static osal_dynlib_lib_handle l_VoskLibHandle = nullptr;

static VoskModel* l_VoskModel           = nullptr;
static VoskRecognizer* l_VoskRecognizer = nullptr;

// VRU implementation variables
//
static bool l_Initialized = false;

static std::vector<VRUWordEntry> l_WordEntries;

static int l_MicState      = 0;
static int l_WordListSize  = 0;
static int l_WordListCount = 0;

static QStringList l_RegisteredWords;
static QList<int>  l_RegisteredWordsIndex;

static SDL_AudioDeviceID l_AudioDevice = 0;
static SDL_AudioSpec     l_AudioDeviceSpec;

//
// Local Functions
//

static bool hook_vosk(void)
{
    std::string debugMessage;

    std::filesystem::path voskApiPath;
    voskApiPath = CoreGetLibraryDirectory();
    voskApiPath += "/libvosk" OSAL_DYNLIB_LIB_EXT_STR;

    l_VoskLibHandle = osal_dynlib_open(voskApiPath);
    if (l_VoskLibHandle == nullptr)
    {
        debugMessage = "VRU: Failed to open library \"";
        debugMessage += voskApiPath.string().c_str();
        debugMessage += "\": ";
        debugMessage += osal_dynlib_strerror();
        PluginDebugMessage(M64MSG_ERROR, debugMessage);
        return false;
    }

    l_vosk_model_new = (ptr_vosk_model_new) osal_dynlib_sym(l_VoskLibHandle, "vosk_model_new");
    l_vosk_model_free = (ptr_vosk_model_free) osal_dynlib_sym(l_VoskLibHandle, "vosk_model_free");
    l_vosk_recognizer_new_grm = (ptr_vosk_recognizer_new_grm) osal_dynlib_sym(l_VoskLibHandle, "vosk_recognizer_new_grm");
    l_vosk_recognizer_free = (ptr_vosk_recognizer_free) osal_dynlib_sym(l_VoskLibHandle, "vosk_recognizer_free");
    l_vosk_recognizer_accept_waveform = (ptr_vosk_recognizer_accept_waveform) osal_dynlib_sym(l_VoskLibHandle, "vosk_recognizer_accept_waveform");
    l_vosk_recognizer_final_result = (ptr_vosk_recognizer_final_result) osal_dynlib_sym(l_VoskLibHandle, "vosk_recognizer_final_result");
    l_vosk_set_log_level = (ptr_vosk_set_log_level) osal_dynlib_sym(l_VoskLibHandle, "vosk_set_log_level");
    l_vosk_recognizer_set_max_alternatives = (ptr_vosk_recognizer_set_max_alternatives) osal_dynlib_sym(l_VoskLibHandle, "vosk_recognizer_set_max_alternatives");

    if (l_vosk_model_new == nullptr ||
        l_vosk_model_free == nullptr ||
        l_vosk_recognizer_new_grm == nullptr ||
        l_vosk_recognizer_free == nullptr ||
        l_vosk_recognizer_accept_waveform == nullptr ||
        l_vosk_recognizer_final_result == nullptr ||
        l_vosk_set_log_level == nullptr ||
        l_vosk_recognizer_set_max_alternatives == nullptr)
    {
        debugMessage = "VRU: Failed to open library: missing functions";
        PluginDebugMessage(M64MSG_ERROR, debugMessage);
        return false;
    }

    return true;
}

static void unhook_vosk(void)
{
    l_vosk_model_new = nullptr;
    l_vosk_model_free = nullptr;
    l_vosk_recognizer_new_grm = nullptr;
    l_vosk_recognizer_free = nullptr;
    l_vosk_recognizer_accept_waveform = nullptr;
    l_vosk_recognizer_final_result = nullptr;
    l_vosk_set_log_level = nullptr;
    l_vosk_recognizer_set_max_alternatives = nullptr;

    osal_dynlib_close(l_VoskLibHandle);
}

static bool setup_vosk_model(void)
{
    std::string debugMessage;

    std::filesystem::path voskModelPath;
    voskModelPath = CoreGetSharedDataDirectory();
    voskModelPath += "/vosk-model-small-en-us-0.15.zip";

    bool missingModelFile = false;

    std::filesystem::path voskModelFiles[] =
    {
        "vosk-model-small-en-us-0.15/am/final.mdl",
        "vosk-model-small-en-us-0.15/conf/mfcc.conf",
        "vosk-model-small-en-us-0.15/conf/model.conf",
        "vosk-model-small-en-us-0.15/graph/disambig_tid.int",
        "vosk-model-small-en-us-0.15/graph/Gr.fst",
        "vosk-model-small-en-us-0.15/graph/HCLr.fst",
        "vosk-model-small-en-us-0.15/graph/phones/word_boundary.int",
        "vosk-model-small-en-us-0.15/ivector/final.dubm",
        "vosk-model-small-en-us-0.15/ivector/final.ie",
        "vosk-model-small-en-us-0.15/ivector/final.mat",
        "vosk-model-small-en-us-0.15/ivector/global_cmvn.stats",
        "vosk-model-small-en-us-0.15/ivector/online_cmvn.conf",
        "vosk-model-small-en-us-0.15/ivector/splice.conf",
    };

    for (std::filesystem::path& file : voskModelFiles)
    {
        std::filesystem::path fullFilePath;
        fullFilePath = CoreGetUserDataDirectory();
        fullFilePath += "/";
        fullFilePath += file;

        if (!std::filesystem::exists(fullFilePath))
        {
            missingModelFile = true;
            break;
        }
    }

    // all required files exist,
    // so we don't need to extract the zip again
    if (!missingModelFile)
    {
        return true;
    }

    if (!std::filesystem::is_regular_file(voskModelPath))
    {
        debugMessage = "VRU: vosk model file \"";
        debugMessage += voskModelPath.string().c_str();
        debugMessage += "\" doesn't exist!";
        PluginDebugMessage(M64MSG_ERROR, debugMessage);
        return false;
    }

    if (!CoreUnzip(voskModelPath, CoreGetUserDataDirectory()))
    {
        debugMessage = "VRU: Failed to extract vosk model file \"";
        debugMessage += voskModelPath.string().c_str();
        debugMessage += "\": ";
        debugMessage += CoreGetError();
        PluginDebugMessage(M64MSG_ERROR, debugMessage);
        return false;
    }

    return true;
}

static bool init_vosk(void)
{
    std::filesystem::path voskModelPath;
    voskModelPath = CoreGetUserDataDirectory();
    voskModelPath += "/vosk-model-small-en-us-0.15";

    l_vosk_set_log_level(-1);

    l_VoskModel = l_vosk_model_new(voskModelPath.string().c_str());
    if (l_VoskModel == nullptr)
    {
        PluginDebugMessage(M64MSG_ERROR, "VRU: vosk_model_new failed");
        return false;
    }

    return true;
}

static void quit_vosk(void)
{
    if (l_VoskModel != nullptr)
    {
        l_vosk_model_free(l_VoskModel);
        l_VoskModel = nullptr;
    }

    if (l_VoskRecognizer != nullptr)
    {
        l_vosk_recognizer_free(l_VoskRecognizer);
        l_VoskRecognizer = nullptr;
    }
}

static bool init_mic(void)
{
    std::string debugMessage;
    SDL_AudioSpec desiredAudioSpec;

    desiredAudioSpec.freq     = 44100;
    desiredAudioSpec.format   = AUDIO_S16SYS;
    desiredAudioSpec.channels = 1;
    desiredAudioSpec.samples  = 1024;
    desiredAudioSpec.callback = nullptr;
    desiredAudioSpec.userdata = nullptr;

    l_AudioDevice = SDL_OpenAudioDevice(nullptr, 1, &desiredAudioSpec, &l_AudioDeviceSpec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (l_AudioDevice == 0)
    {
        debugMessage = "VRU: SDL_OpenAudioDevice Failed: ";
        debugMessage += SDL_GetError();
        PluginDebugMessage(M64MSG_ERROR, debugMessage);
        return false;
    }

    return true;
}

static void quit_mic(void)
{
    if (l_AudioDevice != 0)
    {
        SDL_CloseAudioDevice(l_AudioDevice);
    }
}

static QByteArray words_to_byte_array(uint16_t* words, uint16_t size)
{
    QByteArray byte_array;

    uint8_t* bytes = (uint8_t*)words;

    for (int i = 0; i < (size * 2); i += 2)
    {
        byte_array.push_back(bytes[i + 1]);
        byte_array.push_back(bytes[i]);
    }

    return byte_array;
}

static bool find_word_from_hex(std::string hex, std::string& word)
{
    auto predicate = [hex](const VRUWordEntry& entry)
    {
        return entry.hex == hex;
    };

    auto iter = std::find_if(l_WordEntries.begin(), l_WordEntries.end(), predicate);
    if (iter != l_WordEntries.end())
    {
        word = (*iter).words;
    }

    return iter != l_WordEntries.end();
}

//
// Exported Functions
//

bool InitVRU(void)
{
    if (l_Initialized)
    {
        return true;
    }

    if (!hook_vosk())
    {
        return false;
    }

    if (!setup_vosk_model())
    {
        return false;
    }

    if (!init_vosk())
    {
        return false;
    }

    if (!init_mic())
    {
        return false;
    }

    l_MicState = 0;

    if (l_WordEntries.empty())
    {
        l_WordEntries = GetVRUWordEntries();
    }

    l_Initialized = true;
    return true;
}

bool IsVRUInit(void)
{
    return l_Initialized;
}

bool QuitVRU(void)
{
    if (!l_Initialized)
    {
        return false;
    }

    quit_mic();
    quit_vosk();
    unhook_vosk();

    l_Initialized = false;
    return true;
}

int GetVRUMicState(void)
{
    return l_MicState;
}

//
// VRU Plugin Functions
//

EXPORT void CALL SendVRUWord(uint16_t length, uint16_t* word, uint8_t lang)
{
    if (!l_Initialized)
    {
        return;
    }

    QByteArray byte_array = words_to_byte_array(word, length);
    std::string debugMessage;
    std::string word_str;

    if (find_word_from_hex(byte_array.toHex().toUpper().toStdString(), word_str))
    { // found word
        l_RegisteredWords.append(QString::fromStdString(word_str));
        l_RegisteredWordsIndex.append(l_WordListCount);
    }
    else
    { // not found
        QStringDecoder utf8_decoder = QStringDecoder(QStringDecoder::Utf8);
        QString encoded_string = utf8_decoder.decode(byte_array);

        if (utf8_decoder.hasError())
        {
            debugMessage = "VRU: unknown word: ";
            debugMessage += encoded_string.toStdString();
            PluginDebugMessage(M64MSG_ERROR, debugMessage);
        }
        else
        {
            l_RegisteredWords.append(encoded_string);
            l_RegisteredWordsIndex.append(l_WordListCount);
        }
    }

    l_WordListCount++;

    // we've reached the end of the list
    if (l_WordListCount == l_WordListSize)
    {
        // needed for vosk-api, see vosk_api.h
        l_RegisteredWords.append("[unk]");
        l_RegisteredWordsIndex.append(l_WordListCount);

        QJsonDocument json_document;
        json_document.setArray(QJsonArray::fromStringList(l_RegisteredWords));

        // free existing recognizer if needed
        if (l_VoskRecognizer)
        {
            l_vosk_recognizer_free(l_VoskRecognizer);
            l_VoskRecognizer = nullptr;
        }

        // try to create a new recognizer
        l_VoskRecognizer = l_vosk_recognizer_new_grm(l_VoskModel, (float)l_AudioDeviceSpec.freq, json_document.toJson().constData());
        if (l_VoskRecognizer != nullptr)
        {
            l_vosk_recognizer_set_max_alternatives(l_VoskRecognizer, 3);
        }
    }
}

EXPORT void CALL SetMicState(int state)
{
    if (!l_Initialized)
    {
        return;
    }

    if (state)
    {
        SDL_ClearQueuedAudio(l_AudioDevice);
        SDL_PauseAudioDevice(l_AudioDevice, 0);
    }
    else
    {
        SDL_PauseAudioDevice(l_AudioDevice, 1);
    }

    l_MicState = state;
}

EXPORT void CALL ReadVRUResults(uint16_t* error_flags, uint16_t* num_results, uint16_t* mic_level, uint16_t* voice_level, uint16_t* voice_length, uint16_t* matches)
{
    if (!l_Initialized || l_VoskRecognizer == nullptr)
    {
        return;
    }

    // initialize result data
    *num_results  = 0;
    *mic_level    = 0xBB8;
    *voice_level  = 0xBB8;
    *voice_length = 0x8004;
    for (int i = 0; i < 10; i += 2)
    {
        matches[i]     = 0x7FFF;
        matches[i + 1] = 0;
    }

    // retrieve audio data from device
    SDL_PauseAudioDevice(l_AudioDevice, 1);

    uint32_t audio_size = SDL_GetQueuedAudioSize(l_AudioDevice);
    std::vector<char> audio_buf(audio_size);

    // read audio
    audio_size = SDL_DequeueAudio(l_AudioDevice, audio_buf.data(), audio_buf.size());

    // hand audio data to vosk and retrieve results
    int ret = l_vosk_recognizer_accept_waveform(l_VoskRecognizer, audio_buf.data(), audio_size);
    if (ret == -1)
    {
        *error_flags = 0x8000;
        return;
    }

    QJsonDocument json_document = QJsonDocument::fromJson(l_vosk_recognizer_final_result(l_VoskRecognizer));

    // parse results from vosk
    QJsonArray alternatives_array = json_document.object().value("alternatives").toArray();

    QStringList alternative_words;
    QStringList found_words;
    std::string debugMessage;

    // parse words
    for (int i = 0; i < alternatives_array.size(); i++)
    {
        QString word = alternatives_array.at(i).toObject().value("text").toString();
        if (!word.isEmpty())
        {
            alternative_words.append(word);
        }
    }

    // check the registered words,
    // and see if any matches with the recognized words
    for (const QString& word : l_RegisteredWords)
    {
        if (word != "[unk]" &&
            !found_words.contains(word) && 
            alternative_words.filter(word).size() > 0)
        {
            found_words.append(word);
            if (found_words.size() == 5)
            {
                break;
            }
        }
    }

    // sort found words by length
    std::sort(found_words.begin(), found_words.end(), [](QString& a, QString& b)
    {
        return a.size() > b.size();
    });

    int matches_index = 0;
    for (int i = 0; i < found_words.size(); i++)
    {
        matches[matches_index++] = l_RegisteredWordsIndex.at(l_RegisteredWords.indexOf(found_words.at(i)));
        matches[matches_index++] = i * 256;
        debugMessage = "VRU: found match ";
        debugMessage += std::to_string(i);
        debugMessage += ": ";
        debugMessage += found_words.at(i).toStdString();
        PluginDebugMessage(M64MSG_INFO, debugMessage);
    }

    if (found_words.size() == 0 && alternative_words.size() > 0)
    {
        // heard something but it didn't match anything
        *error_flags = 0x4000;
        matches[0] = 0;
        found_words.append("0");
        debugMessage = "VRU: heard something but it didn't match anything";
        PluginDebugMessage(M64MSG_INFO, debugMessage);
    }

    *num_results = found_words.size();
}

EXPORT void CALL ClearVRUWords(uint8_t length)
{
    if (!l_Initialized)
    {
        return;
    }

    l_WordListCount = 0;
    l_WordListSize  = length;

    l_RegisteredWords.clear();
    l_RegisteredWordsIndex.clear();

    if (l_VoskRecognizer != nullptr)
    {
        l_vosk_recognizer_free(l_VoskRecognizer);
        l_VoskRecognizer = nullptr;
    }
}

EXPORT void CALL SetVRUWordMask(uint8_t length, uint8_t* mask)
{
}

