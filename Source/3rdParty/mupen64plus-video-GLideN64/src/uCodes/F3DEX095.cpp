#include "F3D.h"
#include "F3DEX.h"
#include "F3DEX095.h"
#include "gSP.h"
#include "GBI.h"

// See #2774 for details
void F3DEX095_Init()
{
	F3DEX_Init();
	GBI_SetGBI(G_CULLDL, F3D_CULLDL, F3D_CullDL);
}
