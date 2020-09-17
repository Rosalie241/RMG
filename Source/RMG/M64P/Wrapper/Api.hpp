#ifndef M64P_WRAPPER_API
#define M64P_WRAPPER_API

#include "Core.hpp"
#include "Config.hpp"
#include "Plugin.hpp"
#include "Types.hpp"

#include <QString>

namespace M64P
{
    namespace Wrapper
    {
        class Api
        {
        public:
            Api(void);
            ~Api(void);

            M64P::Wrapper::Core Core;
            M64P::Wrapper::Config Config;

            bool Init(QString);

            QString GetLastError(void);

        private:
            QString error_Message;

            m64p_dynlib_handle core_Handle;
            bool core_Handle_Opened;
            bool core_Handle_Open(QString);
            void core_Handle_Close(void);
        };
    } // namespace Wrapper
} // namespace M64P

#endif // M64P_WRAPPER_API
