/* packmast.cpp --

   This file is part of the UPX executable compressor.

   Copyright (C) 1996-2011 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996-2011 Laszlo Molnar
   All Rights Reserved.

   UPX and the UCL library are free software; you can redistribute them
   and/or modify them under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer              Laszlo Molnar
   <markus@oberhumer.com>               <ml1050@users.sourceforge.net>
 */


#include "conf.h"
#include "file.h"
#include "packmast.h"
#include "packer.h"
#include "lefile.h"
#include "pefile.h"
#include "p_elf.h"

#include "p_com.h"
#include "p_djgpp2.h"
#include "p_exe.h"
#include "p_unix.h"
#include "p_lx_exc.h"
#include "p_lx_elf.h"
//#include "p_lx_sep.h"
#include "p_lx_sh.h"
#include "p_lx_interp.h"
#include "p_sys.h"
#include "p_tos.h"
#include "p_wcle.h"
#include "p_tmt.h"
#include "p_vxd.h"
#include "p_w16ne.h"
#include "p_w32pe.h"
#include "p_vmlinz.h"
#include "p_vmlinx.h"
#include "p_ps1.h"
#include "p_mach.h"
#include "p_armpe.h"


/*************************************************************************
//
**************************************************************************/

PackMaster::PackMaster(InputFile *f, options_t *o) :
    fi(f), p(NULL)
{
    // replace global options with local options
    saved_opt = o;
    if (o)
    {
        memcpy(&this->local_options, o, sizeof(*o)); // struct copy
        opt = &this->local_options;
    }
}


PackMaster::~PackMaster()
{
    fi = NULL;
    delete p; p = NULL;
    // restore global options
    if (saved_opt)
        opt = saved_opt;
    saved_opt = NULL;
}


/*************************************************************************
//
**************************************************************************/

static Packer* try_pack(Packer *p, void *user)
{
    if (p == NULL)
        return NULL;
    InputFile *f = (InputFile *) user;
    p->assertPacker();
    try {
        p->initPackHeader();
        f->seek(0,SEEK_SET);
        if (p->canPack())
        {
            if (opt->cmd == CMD_COMPRESS)
                p->updatePackHeader();
            f->seek(0,SEEK_SET);
            return p;
        }
    } catch (const IOException&) {
    } catch (...) {
        delete p;
        throw;
    }
    delete p;
    return NULL;
}


static Packer* try_unpack(Packer *p, void *user)
{
    if (p == NULL)
        return NULL;
    InputFile *f = (InputFile *) user;
    p->assertPacker();
    try {
        p->initPackHeader();
        f->seek(0,SEEK_SET);
        int r = p->canUnpack();
        if (r > 0)
        {
            f->seek(0,SEEK_SET);
            return p;
        }
        if (r < 0)
        {
            // FIXME - could stop testing all other unpackers at this time
            // see canUnpack() in packer.h
        }
    } catch (const IOException&) {
    } catch (...) {
        delete p;
        throw;
    }
    delete p;
    return NULL;
}


/*************************************************************************
//
**************************************************************************/

Packer* PackMaster::visitAllPackers(visit_func_t func, InputFile *f, const options_t *o, void *user)
{
    Packer *p = NULL;

    // note: order of tries is important !

    //
    // .exe
    //
    if (!o->dos_exe.force_stub)
    {
        if ((p = func(new PackW32Pe(f), user)) != NULL)
            return p;
        delete p; p = NULL;
    }

    return NULL;
}


Packer *PackMaster::getPacker(InputFile *f)
{
    Packer *pp = visitAllPackers(try_pack, f, opt, f);
    if (!pp)
        throwUnknownExecutableFormat();
    pp->assertPacker();
    return pp;
}


Packer *PackMaster::getUnpacker(InputFile *f)
{
    Packer *pp = visitAllPackers(try_unpack, f, opt, f);
    if (!pp)
        throwNotPacked();
    pp->assertPacker();
    return pp;
}


/*************************************************************************
// delegation
**************************************************************************/

void PackMaster::pack(OutputFile *fo)
{
    p = getPacker(fi);
    fi = NULL;
    p->doPack(fo);
}


void PackMaster::unpack(OutputFile *fo)
{
    p = getUnpacker(fi);
    p->assertPacker();
    fi = NULL;
    p->doUnpack(fo);
}


void PackMaster::test()
{
    p = getUnpacker(fi);
    fi = NULL;
    p->doTest();
}


void PackMaster::list()
{
    p = getUnpacker(fi);
    fi = NULL;
    p->doList();
}


void PackMaster::fileInfo()
{
    p = visitAllPackers(try_unpack, fi, opt, fi);
    if (!p)
        p = visitAllPackers(try_pack, fi, opt, fi);
    if (!p)
        throwUnknownExecutableFormat(NULL, 1);    // make a warning here
    p->assertPacker();
    fi = NULL;
    p->doFileInfo();
}


/*
vi:ts=4:et:nowrap
*/

