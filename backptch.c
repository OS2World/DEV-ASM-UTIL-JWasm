/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  backpatch: short forward jump optimization.
*
****************************************************************************/

#include "globals.h"
#include "parser.h"
#include "memalloc.h"
#include "fixup.h"
#include "directiv.h"
#include "segment.h"

/*
 * LABELOPT: short jump label optimization.
 * if this is 0, there is just the simple "fixup backpatch",
 * which cannot adjust any label offsets between the forward reference
 * and the newly defined label, resulting in more passes to be needed.
*/
#define LABELOPT 1

#if 0 /* v1.96: disabled */
#define SkipFixup() \
    fixup->nextbp = sym->fixup; \
    sym->fixup = fixup
#else
#define SkipFixup()
#endif
static ret_code DoPatch( struct asm_sym *sym, struct asmfixup *fixup )
/********************************************************************/
{
    long                disp;
    long                max_disp;
    unsigned            size;
    asm_sym             *sym2;
    dir_node            *seg;

    /* all relative fixups should occure only at first pass and they signal forward references
     * they must be removed after patching or skiped ( next processed as normal fixup )
     */

    DebugMsg(("DoPatch(%u, %s): sym=%s fixup->ofs=%Xh loc=%Xh opt=%u def_seg=%s\n",
              Parse_Pass + 1, sym->name, fixup->sym ? fixup->sym->name : "",
              fixup->offset, fixup->fixup_loc, fixup->option,
              fixup->def_seg ? fixup->def_seg->sym.name : "NULL" ));
    seg = GetSeg( sym );
    if( seg == NULL || fixup->def_seg != seg ) {
        /* if fixup location is in another segment, backpatch is possible, but
         * complicated and it's a pretty rare case, so nothing's done.
         */
        DebugMsg(("DoPatch: skipped due to seg incompat: %s - %s\n",
                  fixup->def_seg ? fixup->def_seg->sym.name : "NULL",
                  seg ? seg->sym.name : "NULL" ));
        SkipFixup();
        return( NOT_ERROR );
    }

    if( Parse_Pass == PASS_1 ) {
        if( sym->mem_type == MT_FAR && fixup->option == OPTJ_CALL ) {
            /* convert near call to push cs + near call,
             * (only at first pass) */
            DebugMsg(("DoPatch: Phase error! caused by far call optimization\n"));
            PhaseError = TRUE;
            sym->offset++;  /* a PUSH CS will be added */
            /* todo: insert LABELOPT block here */
            OutputByte( 0 ); /* it's pass one, nothing is written */
            AsmFree( fixup );
            return( NOT_ERROR );
        //} else if( sym->mem_type == MT_NEAR ) {
        } else {
            // forward reference, only at first pass
            switch( fixup->type ) {
            case FIX_RELOFF32:
            case FIX_RELOFF16:
                AsmFree( fixup );
                return( NOT_ERROR );
            case FIX_LOBYTE:  /* push <forward reference> */
                size = 1;    /* size increases from 2 to 3/5 */
                goto patch;
            }
        }
    }
    size = 0;
    switch( fixup->type ) {
    case FIX_RELOFF32:
        size = 2; /* will be 4 finally */
        /* fall through */
    case FIX_RELOFF16:
        size++; /* will be 2 finally */
        /* fall through */
    case FIX_RELOFF8:
        size++;
        // calculate the displacement
        // disp = fixup->offset + GetCurrOffset() - fixup->fixup_loc - size;
        disp = fixup->offset + fixup->sym->offset - fixup->fixup_loc - size - 1;
        max_disp = (1UL << ((size * 8)-1)) - 1;
        if( disp > max_disp || disp < (-max_disp-1) ) {
        patch:
            DebugMsg(("DoPatch(%u): Phase error, disp=%X, fixup=%s(%X), loc=%X!\n", Parse_Pass + 1, disp, fixup->sym->name, fixup->sym->offset, fixup->fixup_loc ));
            PhaseError = TRUE;
            /* ok, the standard case is: there's a forward jump which
             * was assumed to be SHORT, but it must be NEAR instead.
             */
            switch( size ) {
            case 1:
                size = 0;
                switch( fixup->option ) {
                case OPTJ_EXPLICIT:
#if 0 /* don't display the error at the destination line! */
                    sym->fixup = NULL;
                    DebugMsg(("DoPatch: jump out of range, disp=%d\n", disp ));
                    AsmErr( JUMP_OUT_OF_RANGE, disp - max_disp );
                    return( ERROR );
#else
                    return( NOT_ERROR ); /* nothing to do */
#endif
                case OPTJ_EXTEND: /* Jxx for 8086 */
                    size++;       /* will be 3/5 finally */
                    /* fall through */
                case OPTJ_JXX: /* Jxx for 386 */
                    size++;
                    /* fall through */
                default: /* normal JMP (and PUSH) */
                    // if( CodeInfo->Ofssize ) v1.96: don't use CodeInfo here!
                    if( seg->e.seginfo->Ofssize )
                        size += 2; /* NEAR32 instead of NEAR16 */
                    size++;
                    sym->offset += size;
#if LABELOPT
                    /* scan the segment's label list and adjust all labels
                     * which are between the fixup loc and the current sym.
                     * ( PROCs are NOT contained in this list because they
                     * use the <next>-field of dir_node already!)
                     */
                    DebugMsg(("DoPatch: sym %s, offset changed %X -> %X\n", sym->name, sym->offset - size, sym->offset));
                    for ( sym2 = seg->e.seginfo->labels; sym2; sym2 = (asm_sym *)((dir_node *)sym2)->next ) {
                        if ( sym2 == sym )
                            continue;
                        /* v2.0: fixup_loc is at least 1 byte too low, so
                         * use the "<=" operator instead of "<"!
                         */
                        //if ( sym2->offset < fixup->fixup_loc )
                        if ( sym2->offset <= fixup->fixup_loc )
                            break;
                        sym2->offset += size;
                        DebugMsg(("sym %s, offset changed %X -> %X\n", sym2->name, sym2->offset - size, sym2->offset));
                    }
#endif
                    /*  it doesn't matter what's actually "written" */
                    for (;size;size--)
                        OutputByte( 0xCC );
                    break;
                }
                break;
            case 2:
            case 4:
                DebugMsg(("DoPatch: jump out of range, disp=%d\n", disp ));
                AsmWarn( 4, JUMP_OUT_OF_RANGE, disp - max_disp );
                break;
            }
        }
#ifdef DEBUG_OUT
        else
            DebugMsg(("DoPatch: displacement still short: %Xh\n", disp ));
#endif
        AsmFree( fixup );
        break;
    default:
        DebugMsg(("DoPatch: default branch, unhandled fixup type=%u\n", fixup->type ));
        SkipFixup();
        break;
    }
    return( NOT_ERROR );
}

ret_code BackPatch( struct asm_sym *sym )
/***************************************/
/*
 * patching for forward reference labels in Jmp/Call instructions;
 * called by LabelCreate(), ProcDef() and data_init(), that is, whenever
 * a (new) label is defined. The new label is the <sym> parameter.
 * During the process, the label's offset might be changed!
 *
 * field sym->fixup is a "descending" list of forward references
 * to this symbol. These fixups are only generated during pass 1.
 */
{
    struct asmfixup     *fixup;
    struct asmfixup     *next;

    DebugMsg(("BackPatch(%s) enter, seg.ofs=%s.%X\n", sym->name, CurrSeg->sym.name, sym->offset ));
    fixup = sym->fixup;
    sym->fixup = NULL;
    for( ; fixup != NULL; fixup = next ) {
        next = fixup->nextbp;
        if( DoPatch( sym, fixup ) == ERROR ) {
            return( ERROR );
        }
    }
    DebugMsg(("BackPatch(%s) exit, offset=%X\n", sym->name, sym->offset ));
    return( NOT_ERROR );
}

