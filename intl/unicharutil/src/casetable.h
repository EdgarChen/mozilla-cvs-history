/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "nscore.h" 

extern PRUnichar gUpperToTitle[] = { 
   0x01F1,    0x01F2, 
   0x01CA,    0x01CB, 
   0x01C4,    0x01C5, 
   0x01C7,    0x01C8, 
};

extern PRUint32 gUpperToTitleItems = 4;

extern PRUnichar gToUpper[] = 
{ /*   From    To      Every   Diff   */ 
       0x0061, 0x007a, 0x0001, 0xffe0  ,
       0x00e0, 0x00f6, 0x0001, 0xffe0  ,
       0x00f8, 0x00fe, 0x0001, 0xffe0  ,
       0x00ff, 0x00ff, 0x0001, 0x0079  ,
       0x0101, 0x012f, 0x0002, 0xffff  ,
       0x0131, 0x0131, 0x0002, 0xff18  ,
       0x0133, 0x0137, 0x0002, 0xffff  ,
       0x013a, 0x0148, 0x0002, 0xffff  ,
       0x014b, 0x0177, 0x0002, 0xffff  ,
       0x017a, 0x017e, 0x0002, 0xffff  ,
       0x017f, 0x017f, 0x0001, 0xfed4  ,
       0x0183, 0x0185, 0x0002, 0xffff  ,
       0x0188, 0x018c, 0x0004, 0xffff  ,
       0x0192, 0x0199, 0x0007, 0xffff  ,
       0x01a1, 0x01a5, 0x0002, 0xffff  ,
       0x01a8, 0x01ad, 0x0005, 0xffff  ,
       0x01b0, 0x01b4, 0x0004, 0xffff  ,
       0x01b6, 0x01b9, 0x0003, 0xffff  ,
       0x01bd, 0x01c5, 0x0008, 0xffff  ,
       0x01c6, 0x01c6, 0x0001, 0xfffe  ,
       0x01c8, 0x01c8, 0x0002, 0xffff  ,
       0x01c9, 0x01c9, 0x0001, 0xfffe  ,
       0x01cb, 0x01cb, 0x0002, 0xffff  ,
       0x01cc, 0x01cc, 0x0001, 0xfffe  ,
       0x01ce, 0x01dc, 0x0002, 0xffff  ,
       0x01dd, 0x01dd, 0x0001, 0xffb1  ,
       0x01df, 0x01ef, 0x0002, 0xffff  ,
       0x01f2, 0x01f2, 0x0003, 0xffff  ,
       0x01f3, 0x01f3, 0x0001, 0xfffe  ,
       0x01f5, 0x01fb, 0x0006, 0xffff  ,
       0x01fd, 0x0217, 0x0002, 0xffff  ,
       0x0253, 0x0253, 0x003c, 0xff2e  ,
       0x0254, 0x0254, 0x0001, 0xff32  ,
       0x0256, 0x0257, 0x0001, 0xff33  ,
       0x0259, 0x0259, 0x0002, 0xff36  ,
       0x025b, 0x025b, 0x0002, 0xff35  ,
       0x0260, 0x0260, 0x0005, 0xff33  ,
       0x0263, 0x0263, 0x0003, 0xff31  ,
       0x0268, 0x0268, 0x0005, 0xff2f  ,
       0x0269, 0x026f, 0x0006, 0xff2d  ,
       0x0272, 0x0272, 0x0003, 0xff2b  ,
       0x0275, 0x0275, 0x0003, 0xff2a  ,
       0x0280, 0x0283, 0x0003, 0xff26  ,
       0x0288, 0x0288, 0x0005, 0xff26  ,
       0x028a, 0x028b, 0x0001, 0xff27  ,
       0x0292, 0x0292, 0x0007, 0xff25  ,
       0x0345, 0x0345, 0x00b3, 0x0054  ,
       0x03ac, 0x03ac, 0x0067, 0xffda  ,
       0x03ad, 0x03af, 0x0001, 0xffdb  ,
       0x03b1, 0x03c1, 0x0001, 0xffe0  ,
       0x03c2, 0x03c2, 0x0001, 0xffe1  ,
       0x03c3, 0x03cb, 0x0001, 0xffe0  ,
       0x03cc, 0x03cc, 0x0001, 0xffc0  ,
       0x03cd, 0x03ce, 0x0001, 0xffc1  ,
       0x03d0, 0x03d0, 0x0002, 0xffc2  ,
       0x03d1, 0x03d1, 0x0001, 0xffc7  ,
       0x03d5, 0x03d5, 0x0004, 0xffd1  ,
       0x03d6, 0x03d6, 0x0001, 0xffca  ,
       0x03e3, 0x03ef, 0x0002, 0xffff  ,
       0x03f0, 0x03f0, 0x0001, 0xffaa  ,
       0x03f1, 0x03f1, 0x0001, 0xffb0  ,
       0x03f2, 0x03f2, 0x0001, 0xffb1  ,
       0x0430, 0x044f, 0x0001, 0xffe0  ,
       0x0451, 0x045c, 0x0001, 0xffb0  ,
       0x045e, 0x045f, 0x0001, 0xffb0  ,
       0x0461, 0x0481, 0x0002, 0xffff  ,
       0x0491, 0x04bf, 0x0002, 0xffff  ,
       0x04c2, 0x04c4, 0x0002, 0xffff  ,
       0x04c8, 0x04cc, 0x0004, 0xffff  ,
       0x04d1, 0x04eb, 0x0002, 0xffff  ,
       0x04ef, 0x04f5, 0x0002, 0xffff  ,
       0x04f9, 0x04f9, 0x0004, 0xffff  ,
       0x0561, 0x0586, 0x0001, 0xffd0  ,
       0x1e01, 0x1e95, 0x0002, 0xffff  ,
       0x1e9b, 0x1e9b, 0x0006, 0xffc5  ,
       0x1ea1, 0x1ef9, 0x0002, 0xffff  ,
       0x1f00, 0x1f07, 0x0001, 0x0008  ,
       0x1f10, 0x1f15, 0x0001, 0x0008  ,
       0x1f20, 0x1f27, 0x0001, 0x0008  ,
       0x1f30, 0x1f37, 0x0001, 0x0008  ,
       0x1f40, 0x1f45, 0x0001, 0x0008  ,
       0x1f51, 0x1f57, 0x0002, 0x0008  ,
       0x1f60, 0x1f67, 0x0001, 0x0008  ,
       0x1f70, 0x1f71, 0x0001, 0x004a  ,
       0x1f72, 0x1f75, 0x0001, 0x0056  ,
       0x1f76, 0x1f77, 0x0001, 0x0064  ,
       0x1f78, 0x1f79, 0x0001, 0x0080  ,
       0x1f7a, 0x1f7b, 0x0001, 0x0070  ,
       0x1f7c, 0x1f7d, 0x0001, 0x007e  ,
       0x1f80, 0x1f87, 0x0001, 0x0008  ,
       0x1f90, 0x1f97, 0x0001, 0x0008  ,
       0x1fa0, 0x1fa7, 0x0001, 0x0008  ,
       0x1fb0, 0x1fb1, 0x0001, 0x0008  ,
       0x1fb3, 0x1fb3, 0x0002, 0x0009  ,
       0x1fbe, 0x1fbe, 0x000b, 0xe3db  ,
       0x1fc3, 0x1fc3, 0x0005, 0x0009  ,
       0x1fd0, 0x1fd1, 0x0001, 0x0008  ,
       0x1fe0, 0x1fe1, 0x0001, 0x0008  ,
       0x1fe5, 0x1fe5, 0x0004, 0x0007  ,
       0x1ff3, 0x1ff3, 0x000e, 0x0009  ,
       0x2170, 0x217f, 0x0001, 0xfff0  ,
       0x24d0, 0x24e9, 0x0001, 0xffe6  ,
       0xff41, 0xff5a, 0x0001, 0xffe0  
};

extern PRUint32 gToUpperItems = 103;

extern PRUnichar gToLower[] = 
{ /*   From    To      Every   Diff   */ 
       0x0041, 0x005a, 0x0001, 0x0020  ,
       0x00c0, 0x00d6, 0x0001, 0x0020  ,
       0x00d8, 0x00de, 0x0001, 0x0020  ,
       0x0100, 0x012e, 0x0002, 0x0001  ,
       0x0130, 0x0130, 0x0002, 0xff39  ,
       0x0132, 0x0136, 0x0002, 0x0001  ,
       0x0139, 0x0147, 0x0002, 0x0001  ,
       0x014a, 0x0176, 0x0002, 0x0001  ,
       0x0178, 0x0178, 0x0002, 0xff87  ,
       0x0179, 0x017d, 0x0002, 0x0001  ,
       0x0181, 0x0181, 0x0004, 0x00d2  ,
       0x0182, 0x0184, 0x0002, 0x0001  ,
       0x0186, 0x0186, 0x0002, 0x00ce  ,
       0x0187, 0x0187, 0x0001, 0x0001  ,
       0x0189, 0x018a, 0x0001, 0x00cd  ,
       0x018b, 0x018b, 0x0001, 0x0001  ,
       0x018e, 0x018e, 0x0003, 0x004f  ,
       0x018f, 0x018f, 0x0001, 0x00ca  ,
       0x0190, 0x0190, 0x0001, 0x00cb  ,
       0x0191, 0x0191, 0x0001, 0x0001  ,
       0x0193, 0x0193, 0x0002, 0x00cd  ,
       0x0194, 0x0194, 0x0001, 0x00cf  ,
       0x0196, 0x0196, 0x0002, 0x00d3  ,
       0x0197, 0x0197, 0x0001, 0x00d1  ,
       0x0198, 0x0198, 0x0001, 0x0001  ,
       0x019c, 0x019c, 0x0004, 0x00d3  ,
       0x019d, 0x019d, 0x0001, 0x00d5  ,
       0x019f, 0x019f, 0x0002, 0x00d6  ,
       0x01a0, 0x01a4, 0x0002, 0x0001  ,
       0x01a6, 0x01a6, 0x0002, 0x00da  ,
       0x01a7, 0x01a7, 0x0001, 0x0001  ,
       0x01a9, 0x01a9, 0x0002, 0x00da  ,
       0x01ac, 0x01ac, 0x0003, 0x0001  ,
       0x01ae, 0x01ae, 0x0002, 0x00da  ,
       0x01af, 0x01af, 0x0001, 0x0001  ,
       0x01b1, 0x01b2, 0x0001, 0x00d9  ,
       0x01b3, 0x01b5, 0x0002, 0x0001  ,
       0x01b7, 0x01b7, 0x0002, 0x00db  ,
       0x01b8, 0x01bc, 0x0004, 0x0001  ,
       0x01c4, 0x01c4, 0x0008, 0x0002  ,
       0x01c5, 0x01c5, 0x0001, 0x0001  ,
       0x01c7, 0x01c7, 0x0002, 0x0002  ,
       0x01c8, 0x01c8, 0x0001, 0x0001  ,
       0x01ca, 0x01ca, 0x0002, 0x0002  ,
       0x01cb, 0x01db, 0x0002, 0x0001  ,
       0x01de, 0x01ee, 0x0002, 0x0001  ,
       0x01f1, 0x01f1, 0x0003, 0x0002  ,
       0x01f2, 0x01f4, 0x0002, 0x0001  ,
       0x01fa, 0x0216, 0x0002, 0x0001  ,
       0x0386, 0x0386, 0x0170, 0x0026  ,
       0x0388, 0x038a, 0x0001, 0x0025  ,
       0x038c, 0x038c, 0x0002, 0x0040  ,
       0x038e, 0x038f, 0x0001, 0x003f  ,
       0x0391, 0x03a1, 0x0001, 0x0020  ,
       0x03a3, 0x03ab, 0x0001, 0x0020  ,
       0x03e2, 0x03ee, 0x0002, 0x0001  ,
       0x0401, 0x040c, 0x0001, 0x0050  ,
       0x040e, 0x040f, 0x0001, 0x0050  ,
       0x0410, 0x042f, 0x0001, 0x0020  ,
       0x0460, 0x0480, 0x0002, 0x0001  ,
       0x0490, 0x04be, 0x0002, 0x0001  ,
       0x04c1, 0x04c3, 0x0002, 0x0001  ,
       0x04c7, 0x04cb, 0x0004, 0x0001  ,
       0x04d0, 0x04ea, 0x0002, 0x0001  ,
       0x04ee, 0x04f4, 0x0002, 0x0001  ,
       0x04f8, 0x04f8, 0x0004, 0x0001  ,
       0x0531, 0x0556, 0x0001, 0x0030  ,
       0x10a0, 0x10c5, 0x0001, 0x0030  ,
       0x1e00, 0x1e94, 0x0002, 0x0001  ,
       0x1ea0, 0x1ef8, 0x0002, 0x0001  ,
       0x1f08, 0x1f0f, 0x0001, 0xfff8  ,
       0x1f18, 0x1f1d, 0x0001, 0xfff8  ,
       0x1f28, 0x1f2f, 0x0001, 0xfff8  ,
       0x1f38, 0x1f3f, 0x0001, 0xfff8  ,
       0x1f48, 0x1f4d, 0x0001, 0xfff8  ,
       0x1f59, 0x1f5f, 0x0002, 0xfff8  ,
       0x1f68, 0x1f6f, 0x0001, 0xfff8  ,
       0x1f88, 0x1f8f, 0x0001, 0xfff8  ,
       0x1f98, 0x1f9f, 0x0001, 0xfff8  ,
       0x1fa8, 0x1faf, 0x0001, 0xfff8  ,
       0x1fb8, 0x1fb9, 0x0001, 0xfff8  ,
       0x1fba, 0x1fbb, 0x0001, 0xffb6  ,
       0x1fbc, 0x1fbc, 0x0001, 0xfff7  ,
       0x1fc8, 0x1fcb, 0x0001, 0xffaa  ,
       0x1fcc, 0x1fcc, 0x0001, 0xfff7  ,
       0x1fd8, 0x1fd9, 0x0001, 0xfff8  ,
       0x1fda, 0x1fdb, 0x0001, 0xff9c  ,
       0x1fe8, 0x1fe9, 0x0001, 0xfff8  ,
       0x1fea, 0x1feb, 0x0001, 0xff90  ,
       0x1fec, 0x1fec, 0x0001, 0xfff9  ,
       0x1ff8, 0x1ff9, 0x0001, 0xff80  ,
       0x1ffa, 0x1ffb, 0x0001, 0xff82  ,
       0x1ffc, 0x1ffc, 0x0001, 0xfff7  ,
       0x2160, 0x216f, 0x0001, 0x0010  ,
       0x24b6, 0x24cf, 0x0001, 0x001a  ,
       0xff21, 0xff3a, 0x0001, 0x0020  
};

extern PRUint32 gToLowerItems = 96;

