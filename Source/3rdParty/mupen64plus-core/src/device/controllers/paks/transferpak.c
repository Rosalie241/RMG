/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - transferpak.c                                           *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* This work is based on the NRage plugin source.
 * Transfer pak emulation is certainly NOT accurate
 * but without proper reverse-engineering this as good as we can get.
 */

#include "transferpak.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"

#include "device/gb/gb_cart.h"
#include "device/controllers/game_controller.h"

#include <string.h>


static uint16_t gb_cart_address(unsigned int bank, uint16_t address)
{
    return 0x4000 * bank + (address & 0x7fff) - 0x4000;
}

void init_transferpak(struct transferpak* tpk, struct gb_cart* gb_cart)
{
    tpk->gb_cart = gb_cart;
}

void poweron_transferpak(struct transferpak* tpk)
{
    tpk->enabled = 0;
    tpk->bank = 0;
    tpk->reset_state = 3;

    if (tpk->gb_cart != NULL) {
        poweron_gb_cart(tpk->gb_cart);
    }
}

void change_gb_cart(struct transferpak* tpk, struct gb_cart* gb_cart)
{
    tpk->enabled = 0;

    if (gb_cart == NULL) {
        tpk->access_mode = CART_NOT_INSERTED;
    }
    else {
        tpk->access_mode = CART_ACCESS_MODE_0;
       // poweron_gb_cart(gb_cart);
    }

    
    tpk->reset_state = 3;
    tpk->gb_cart = gb_cart;
}

static void plug_transferpak(void* pak)
{
    struct transferpak* tpk = (struct transferpak*)pak;
    poweron_transferpak(tpk);
}

static void unplug_transferpak(void* pak)
{
}

 /* Reads from the Transfer Pak. */
 static void read_transferpak(void* pak, uint16_t address, uint8_t* data, size_t size)
 {
     struct transferpak* tpk = (struct transferpak*)pak;
     uint8_t value;
     

 
     switch (address >> 12)
     {
     case 0x8: 
         /*
          * 0x8 => read the TPak "enabled" status 
          * In Rust code: if (enabled) => 0x84 else => 0x00
          */
         value = (tpk->enabled) ? 0x84 : 0x00;
         DebugMessage(M64MSG_VERBOSE, "TPak returning cart state: %02x", value);
         memset(data, value, size);
         return;
         
    default:
        if (!tpk->enabled) {
            DebugMessage(M64MSG_VERBOSE, "TPak read ignored because disabled");
            return;
        }
    }

    switch (address >> 12)
    {
     case 0xB:
     {
         uint8_t val = 0;
 
         if (tpk->gb_cart && tpk->gb_cart->enabled) {
             val |= (1 << 0); /* bit0 => cart enabled? */
         }
         val |= (uint8_t)((tpk->reset_state & 3) << 2);
 
         if (tpk->enabled) {
             val |= (1 << 7);
         }
 
         /* Simple state machine for reset_state (mirroring Rust logic). */
         if ( tpk->gb_cart->enabled && tpk->reset_state == 3) {
             tpk->reset_state = 2;
         }
         else if (!tpk->gb_cart->enabled && tpk->reset_state == 2) {
             tpk->reset_state = 1;
         }
         else if (!tpk->gb_cart->enabled && tpk->reset_state == 1) {
             tpk->reset_state = 0;
         }
 
         DebugMessage(M64MSG_VERBOSE, "TPak read 0xB => %02x", val);
         
         memset(data, val, size);
         break;
     }
 
     case 0xC:
     case 0xD:
     case 0xE:
     case 0xF:
         /*
          * Read from the Game Boy cart.
          * The address is computed as bank-based offset
          * (0x4000 * bank + ((address & 0x7FFF) - 0x4000)).
          */
         if (tpk->gb_cart != NULL)
         {
             const uint16_t cart_addr = gb_cart_address(tpk->bank, address);
             read_gb_cart(tpk->gb_cart, cart_addr, data, size);
            //DebugMessage(M64MSG_VERBOSE, 
             //             "TPak reading cart: bank=%d, raw_addr=%04x => gb_addr=%04x -> %02x", tpk->bank, address, cart_addr, data[0]);
             /*for(int i = 0; i < size; i++)
             {
                 DebugMessage(M64MSG_VERBOSE, "TPak reading cart: bank=%d, raw_addr=%04x => gb_addr=%04x -> %02x", tpk->bank, address, cart_addr, data[i]);
             }*/
            
         }
         else
         {
             DebugMessage(M64MSG_WARNING, "TPak read: no GB cart present");
             memset(data, 0x00, size);
         }
         break;
 
     default:
         DebugMessage(M64MSG_WARNING, "Unknown TPak read at %04x", address);
         memset(data, 0x00, size);
         break;
     }
 }
 
 /* Writes to the Transfer Pak. */
 static void write_transferpak(void* pak, uint16_t address, const uint8_t* buf, size_t size)
 {
     struct transferpak* tpk = (struct transferpak*)pak;
     /* The Rust code always uses the last byte as the "value" for multi-byte writes. */
     uint16_t value = buf[size - 1];
 
     DebugMessage(M64MSG_VERBOSE, "TransferPak write: %04x <- %02x", address, value);
 
     switch (address >> 12)
     {
     case 0x8:
         /*
          * Matches the Rust approach: 0xFE => disable, 0x84 => enable
          */
         switch (value)
         {
         case 0xFE:
             tpk->enabled = 0;
             DebugMessage(M64MSG_VERBOSE, "TPak disabled");
             return;
 
         case 0x84:
             if (!tpk->enabled) 
             {
                 tpk->bank        = 3;
                 tpk->reset_state = 0;
 
                 if (tpk->gb_cart != NULL)
                 {
                   tpk->gb_cart->enabled = 0;
                 }
             }
             tpk->enabled = 1;
             DebugMessage(M64MSG_VERBOSE, "TPak enabled (0x84)");
             return;
 
         default:
             DebugMessage(M64MSG_WARNING, 
                          "Unknown write to TPak (0x8 region): %02x", value);
            return;
         }
         return;
 
     default:
         /* If TPak not enabled, ignore writes (as in your Rust example). */
         if (!tpk->enabled) {
             DebugMessage(M64MSG_VERBOSE, "TPak write ignored because disabled");
             return;
         }
         break;
     case 0xA:
        if (!tpk->enabled) {
            DebugMessage(M64MSG_VERBOSE, "TPak write ignored because disabled");
            return;
        }
         tpk->bank = (uint16_t)value;
         if (tpk->bank > 3) {
             DebugMessage(M64MSG_WARNING, "TPak: invalid bank %d", tpk->bank);
             tpk->bank = 0;
         }
         DebugMessage(M64MSG_VERBOSE, "TPak set bank => %d", tpk->bank);
         break;
 
     case 0xB:
         if (value & 1)
         {
             if (tpk->gb_cart != NULL && !tpk->gb_cart->enabled)
             {
                 /* Reset state to 3, and do any "power-on" type logic on the cart. */
                 tpk->reset_state          = 3;
                 tpk->gb_cart->enabled     = 1;
                 tpk->gb_cart->rom_bank = 1;
                 tpk->gb_cart->ram_bank = 0;
                 tpk->gb_cart->ram_enable = 0;
 
                 /* e.g. set cart type if needed:
                  * tpk->gb_cart->cart_type = get_cart_type(tpk->gb_cart->rom[0x147]);
                  * (This depends on your real code.)
                  */
                 DebugMessage(M64MSG_VERBOSE, "TPak: cart enabled, reset_state=3");
             }
             else if (tpk->gb_cart) 
             {
                 tpk->gb_cart->enabled = 1;
                 DebugMessage(M64MSG_VERBOSE, "TPak: cart already enabled");
             }
         }
         else
         {
             if (tpk->gb_cart) {
                 tpk->gb_cart->enabled = 0;
             }
             DebugMessage(M64MSG_VERBOSE, "TPak: cart disabled via write to 0xB");
         }
         break;
 
     case 0xC:
     case 0xD:
     case 0xE:
     case 0xF:
     {
        if (!tpk->enabled) {
            DebugMessage(M64MSG_VERBOSE, "TPak write ignored because disabled");
            return;
        }
         /* Write to GB cart memory. */
         if (tpk->gb_cart != NULL)
         {
             uint16_t cart_addr = gb_cart_address(tpk->bank, address);
             DebugMessage(M64MSG_VERBOSE, 
                          "TPak write to cart: bank=%d, raw_addr=%04x => gb_addr=%04x val=%02x",
                          tpk->bank, address, cart_addr, value);
             write_gb_cart(tpk->gb_cart, cart_addr, buf, size);
         }
         else
         {
             DebugMessage(M64MSG_WARNING, 
                          "TPak write to 0xC..0xF, but no GB cart present");
         }
         break;
     };
     }
 }
 

/* Transfer pak definition */
const struct pak_interface g_itransferpak =
{
    "Transfer pak",
    plug_transferpak,
    unplug_transferpak,
    read_transferpak,
    write_transferpak
};

