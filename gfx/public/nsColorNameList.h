/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/******

  This file contains the list of all named colors
  See nsCSSColorNames.h for access to the enum values for colors

  It is designed to be used as inline input to nsCSSColorNames.cpp *only*
  through the magic of C preprocessing.

  All entires must be enclosed in the macro GFX_COLOR which will have cruel
  and unusual things done to it

  It is recommended (but not strictly necessary) to keep all entries
  in alphabetical order

  The first argument to GFX_COLOR is both the enum identifier of the color
  and the string value
  The second argument is the sRGBA value for the named color

 ******/


GFX_COLOR(aliceblue, NS_RGB(240, 248, 255))
GFX_COLOR(antiquewhite, NS_RGB(250, 235, 215))
GFX_COLOR(aqua, NS_RGB(  0, 255, 255))
GFX_COLOR(aquamarine, NS_RGB(127, 255, 212))
GFX_COLOR(azure, NS_RGB(240, 255, 255))
GFX_COLOR(beige, NS_RGB(245, 245, 220))
GFX_COLOR(bisque, NS_RGB(255, 228, 196))
GFX_COLOR(black, NS_RGB(  0,   0,   0))
GFX_COLOR(blanchedalmond, NS_RGB(255, 235, 205))
GFX_COLOR(blue, NS_RGB(  0,   0, 255))
GFX_COLOR(blueviolet, NS_RGB(138,  43, 226))
GFX_COLOR(brown, NS_RGB(165,  42,  42))
GFX_COLOR(burlywood, NS_RGB(222, 184, 135))
GFX_COLOR(cadetblue, NS_RGB( 95, 158, 160))
GFX_COLOR(chartreuse, NS_RGB(127, 255,   0))
GFX_COLOR(chocolate, NS_RGB(210, 105,  30))
GFX_COLOR(coral, NS_RGB(255, 127,  80))
GFX_COLOR(cornflowerblue, NS_RGB(100, 149, 237))
GFX_COLOR(cornsilk, NS_RGB(255, 248, 220))
GFX_COLOR(crimson, NS_RGB(220,  20,  60))
GFX_COLOR(cyan, NS_RGB(  0, 255, 255))
GFX_COLOR(darkblue, NS_RGB(  0,   0, 139))
GFX_COLOR(darkcyan, NS_RGB(  0, 139, 139))
GFX_COLOR(darkgoldenrod, NS_RGB(184, 134,  11))
GFX_COLOR(darkgray, NS_RGB(169, 169, 169))
GFX_COLOR(darkgreen, NS_RGB(  0, 100,   0))
GFX_COLOR(darkgrey, NS_RGB(169, 169, 169))
GFX_COLOR(darkkhaki, NS_RGB(189, 183, 107))
GFX_COLOR(darkmagenta, NS_RGB(139,   0, 139))
GFX_COLOR(darkolivegreen, NS_RGB( 85, 107,  47))
GFX_COLOR(darkorange, NS_RGB(255, 140,   0))
GFX_COLOR(darkorchid, NS_RGB(153,  50, 204))
GFX_COLOR(darkred, NS_RGB(139,   0,   0))
GFX_COLOR(darksalmon, NS_RGB(233, 150, 122))
GFX_COLOR(darkseagreen, NS_RGB(143, 188, 143))
GFX_COLOR(darkslateblue, NS_RGB( 72,  61, 139))
GFX_COLOR(darkslategray, NS_RGB( 47,  79,  79))
GFX_COLOR(darkslategrey, NS_RGB( 47,  79,  79))
GFX_COLOR(darkturquoise, NS_RGB(  0, 206, 209))
GFX_COLOR(darkviolet, NS_RGB(148,   0, 211))
GFX_COLOR(deeppink, NS_RGB(255,  20, 147))
GFX_COLOR(deepskyblue, NS_RGB(  0, 191, 255))
GFX_COLOR(dimgray, NS_RGB(105, 105, 105))
GFX_COLOR(dimgrey, NS_RGB(105, 105, 105))
GFX_COLOR(dodgerblue, NS_RGB( 30, 144, 255))
GFX_COLOR(firebrick, NS_RGB(178,  34,  34))
GFX_COLOR(floralwhite, NS_RGB(255, 250, 240))
GFX_COLOR(forestgreen, NS_RGB( 34, 139,  34))
GFX_COLOR(fuchsia, NS_RGB(255,   0, 255))
GFX_COLOR(gainsboro, NS_RGB(220, 220, 220))
GFX_COLOR(ghostwhite, NS_RGB(248, 248, 255))
GFX_COLOR(gold, NS_RGB(255, 215,   0))
GFX_COLOR(goldenrod, NS_RGB(218, 165,  32))
GFX_COLOR(gray, NS_RGB(128, 128, 128))
GFX_COLOR(grey, NS_RGB(128, 128, 128))
GFX_COLOR(green, NS_RGB(  0, 128,   0))
GFX_COLOR(greenyellow, NS_RGB(173, 255,  47))
GFX_COLOR(honeydew, NS_RGB(240, 255, 240))
GFX_COLOR(hotpink, NS_RGB(255, 105, 180))
GFX_COLOR(indianred, NS_RGB(205,  92,  92))
GFX_COLOR(indigo, NS_RGB( 75,   0, 130))
GFX_COLOR(ivory, NS_RGB(255, 255, 240))
GFX_COLOR(khaki, NS_RGB(240, 230, 140))
GFX_COLOR(lavender, NS_RGB(230, 230, 250))
GFX_COLOR(lavenderblush, NS_RGB(255, 240, 245))
GFX_COLOR(lawngreen, NS_RGB(124, 252,   0))
GFX_COLOR(lemonchiffon, NS_RGB(255, 250, 205))
GFX_COLOR(lightblue, NS_RGB(173, 216, 230))
GFX_COLOR(lightcoral, NS_RGB(240, 128, 128))
GFX_COLOR(lightcyan, NS_RGB(224, 255, 255))
GFX_COLOR(lightgoldenrodyellow, NS_RGB(250, 250, 210))
GFX_COLOR(lightgray, NS_RGB(211, 211, 211))
GFX_COLOR(lightgreen, NS_RGB(144, 238, 144))
GFX_COLOR(lightgrey, NS_RGB(211, 211, 211))
GFX_COLOR(lightpink, NS_RGB(255, 182, 193))
GFX_COLOR(lightsalmon, NS_RGB(255, 160, 122))
GFX_COLOR(lightseagreen, NS_RGB( 32, 178, 170))
GFX_COLOR(lightskyblue, NS_RGB(135, 206, 250))
GFX_COLOR(lightslategray, NS_RGB(119, 136, 153))
GFX_COLOR(lightslategrey, NS_RGB(119, 136, 153))
GFX_COLOR(lightsteelblue, NS_RGB(176, 196, 222))
GFX_COLOR(lightyellow, NS_RGB(255, 255, 224))
GFX_COLOR(lime, NS_RGB(  0, 255,   0))
GFX_COLOR(limegreen, NS_RGB( 50, 205,  50))
GFX_COLOR(linen, NS_RGB(250, 240, 230))
GFX_COLOR(magenta, NS_RGB(255,   0, 255))
GFX_COLOR(maroon, NS_RGB(128,   0,   0))
GFX_COLOR(mediumaquamarine, NS_RGB(102, 205, 170))
GFX_COLOR(mediumblue, NS_RGB(  0,   0, 205))
GFX_COLOR(mediumorchid, NS_RGB(186,  85, 211))
GFX_COLOR(mediumpurple, NS_RGB(147, 112, 219))
GFX_COLOR(mediumseagreen, NS_RGB( 60, 179, 113))
GFX_COLOR(mediumslateblue, NS_RGB(123, 104, 238))
GFX_COLOR(mediumspringgreen, NS_RGB(  0, 250, 154))
GFX_COLOR(mediumturquoise, NS_RGB( 72, 209, 204))
GFX_COLOR(mediumvioletred, NS_RGB(199,  21, 133))
GFX_COLOR(midnightblue, NS_RGB( 25,  25, 112))
GFX_COLOR(mintcream, NS_RGB(245, 255, 250))
GFX_COLOR(mistyrose, NS_RGB(255, 228, 225))
GFX_COLOR(moccasin, NS_RGB(255, 228, 181))
GFX_COLOR(navajowhite, NS_RGB(255, 222, 173))
GFX_COLOR(navy, NS_RGB(  0,   0, 128))
GFX_COLOR(oldlace, NS_RGB(253, 245, 230))
GFX_COLOR(olive, NS_RGB(128, 128,   0))
GFX_COLOR(olivedrab, NS_RGB(107, 142,  35))
GFX_COLOR(orange, NS_RGB(255, 165,   0))
GFX_COLOR(orangered, NS_RGB(255,  69,   0))
GFX_COLOR(orchid, NS_RGB(218, 112, 214))
GFX_COLOR(palegoldenrod, NS_RGB(238, 232, 170))
GFX_COLOR(palegreen, NS_RGB(152, 251, 152))
GFX_COLOR(paleturquoise, NS_RGB(175, 238, 238))
GFX_COLOR(palevioletred, NS_RGB(219, 112, 147))
GFX_COLOR(papayawhip, NS_RGB(255, 239, 213))
GFX_COLOR(peachpuff, NS_RGB(255, 218, 185))
GFX_COLOR(peru, NS_RGB(205, 133,  63))
GFX_COLOR(pink, NS_RGB(255, 192, 203))
GFX_COLOR(plum, NS_RGB(221, 160, 221))
GFX_COLOR(powderblue, NS_RGB(176, 224, 230))
GFX_COLOR(purple, NS_RGB(128,   0, 128))
GFX_COLOR(red, NS_RGB(255,   0,   0))
GFX_COLOR(rosybrown, NS_RGB(188, 143, 143))
GFX_COLOR(royalblue, NS_RGB( 65, 105, 225))
GFX_COLOR(saddlebrown, NS_RGB(139,  69,  19))
GFX_COLOR(salmon, NS_RGB(250, 128, 114))
GFX_COLOR(sandybrown, NS_RGB(244, 164,  96))
GFX_COLOR(seagreen, NS_RGB( 46, 139,  87))
GFX_COLOR(seashell, NS_RGB(255, 245, 238))
GFX_COLOR(sienna, NS_RGB(160,  82,  45))
GFX_COLOR(silver, NS_RGB(192, 192, 192))
GFX_COLOR(skyblue, NS_RGB(135, 206, 235))
GFX_COLOR(slateblue, NS_RGB(106,  90, 205))
GFX_COLOR(slategray, NS_RGB(112, 128, 144))
GFX_COLOR(slategrey, NS_RGB(112, 128, 144))
GFX_COLOR(snow, NS_RGB(255, 250, 250))
GFX_COLOR(springgreen, NS_RGB(  0, 255, 127))
GFX_COLOR(steelblue, NS_RGB( 70, 130, 180))
GFX_COLOR(tan, NS_RGB(210, 180, 140))
GFX_COLOR(teal, NS_RGB(  0, 128, 128))
GFX_COLOR(thistle, NS_RGB(216, 191, 216))
GFX_COLOR(tomato, NS_RGB(255,  99,  71))
GFX_COLOR(turquoise, NS_RGB( 64, 224, 208))
GFX_COLOR(violet, NS_RGB(238, 130, 238))
GFX_COLOR(wheat, NS_RGB(245, 222, 179))
GFX_COLOR(white, NS_RGB(255, 255, 255))
GFX_COLOR(whitesmoke, NS_RGB(245, 245, 245))
GFX_COLOR(yellow, NS_RGB(255, 255,   0))
GFX_COLOR(yellowgreen, NS_RGB(154, 205,  50))

