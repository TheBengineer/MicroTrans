/* $Id: sigs.c,v 1.44 2015/05/19 17:47:37 jsm Exp $ */
#include "trans.h"

const char RevNum[SIZE_OF_REVNUM] =  {    // revision no:
 // only change for major rev and/or interface change.
  "Trans 00028.7      "
},
 Signature[SIZE_OF_SIGNATURE] = {            // program title.
 // Change this every time you tweak a feature.
  "Trans controller 1.0.1         20150519 18:43GMT(c)JSM "
//"1234567890123456789012345678901234567890123456789012345"
 };
