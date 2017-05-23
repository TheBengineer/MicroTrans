/* $Id: trans_main_decls.h,v 1.4 2015/01/11 14:02:44 jsm Exp $ */
// this gives warnings in gcc which is annoying, but using it does make some things easier
// now used by CAN and SCI routines that reference tables
// has a slightly different usage to MS2 2.8+
const tableDescriptor tables[NO_TBLES] =  { 
  { (unsigned int *)cltfactor_table,    (unsigned int *)cltfactor_table,    sizeof(cltfactor_table) }, 
  { (unsigned int *)matfactor_table,    (unsigned int *)matfactor_table,    sizeof(matfactor_table) }, 
  { NULL,                               NULL,                               0                       },
  { NULL,                               NULL,                               0                       },
  { (unsigned int *)&ram_data,          (unsigned int *)&flash4,            1024                    }, 
  { NULL,                               NULL,                               0                       },
  { (unsigned int *)&canbuf,            NULL,                               sizeof(canbuf)          },
  { (unsigned int *)&outpc,             NULL,                               sizeof(outpc)           },
  { NULL,                               NULL,                               0                       },
  { NULL,                               NULL,                               0                       },
  { NULL,                               NULL,                               0                       },
  { NULL,                               NULL,                               0                       },
  { NULL,                               NULL,                               0                       },
  { NULL,                               NULL,                               0                       },
  { (unsigned int *)&Signature,         (unsigned int *)&Signature,         60                      },
  { (unsigned int *)&RevNum,            (unsigned int *)&RevNum,            20                      }
};

const unsigned int twopow[16] = {1,2,4,8,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800,0x1000,0x2000,0x4000,0x8000};

const unsigned int builtingears[16][10] = { /* Common ratios for each transmission */
   {248,148,100,75, 100,100,100,100,100,100}, /* 4L80E */
   {306,163,100,70, 100,100,100,100,100,100}, /* 4L60E */
   {253,153,100,71, 100,100,100,100,100,100}, /* A341E */
   {282,157,100,69, 100,100,100,100,100,100}, /* 41TE */
   {284,155,100,70, 100,100,100,100,100,100}, /* 4R70W */
   {284,155,100,70, 100,100,100,100,100,100}, /* 4R70W */
   {295,162,100,68, 100,100,100,100,100,100}, /* 4T40E */
   {342,221,160,100, 75,100,100,100,100,100}, /* 5L40E */
   {271,154,100,71, 100,100,100,100,100,100}, /* E4OD */
   {271,154,100,71, 100,100,100,100,100,100}, /* E4OD */
   {255,149,100,69, 100,100,100,100,100,100}, /* W4A33 */
   {248,148,100,75, 100,100,100,100,100,100}, /* spare */
   {248,148,100,75, 100,100,100,100,100,100}, /* spare */
   {248,148,100,75, 100,100,100,100,100,100}, /* spare */
   {248,148,100,75, 100,100,100,100,100,100}, /* spare */
   {248,148,100,75, 100,100,100,100,100,100}  /* spare */
    };
