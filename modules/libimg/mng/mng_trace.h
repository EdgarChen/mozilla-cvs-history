/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : mng_trace.h               copyright (c) 2000 G.Juyn        * */
/* * version   : 0.9.0                                                      * */
/* *                                                                        * */
/* * purpose   : Trace functions (definition)                               * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* * web       : http://www.3-t.com                                         * */
/* * email     : mailto:info@3-t.com                                        * */
/* *                                                                        * */
/* * comment   : Definition of the trace functions                          * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - added chunk-access function trace-codes                  * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/12/2000 - G.Juyn                                * */
/* *             - changed trace to macro for callback error-reporting      * */
/* *             0.5.1 - 05/13/2000 - G.Juyn                                * */
/* *             - added save_state & restore_state trace-codes             * */
/* *             0.5.1 - 05/15/2000 - G.Juyn                                * */
/* *             - added getimgdata & putimgdata trace-codes                * */
/* *                                                                        * */
/* *             0.5.2 - 05/20/2000 - G.Juyn                                * */
/* *             - added JNG tracecodes                                     * */
/* *             0.5.2 - 05/23/2000 - G.Juyn                                * */
/* *             - added trace-table entry definition                       * */
/* *             0.5.2 - 05/24/2000 - G.Juyn                                * */
/* *             - added tracecodes for global animation color-chunks       * */
/* *             - added tracecodes for get/set of default ZLIB/IJG parms   * */
/* *             - added tracecodes for global PLTE,tRNS,bKGD               * */
/* *             0.5.2 - 05/30/2000 - G.Juyn                                * */
/* *             - added tracecodes for image-object promotion              * */
/* *             - added tracecodes for delta-image processing              * */
/* *             0.5.2 - 06/02/2000 - G.Juyn                                * */
/* *             - added tracecodes for getalphaline callback               * */
/* *             0.5.2 - 06/05/2000 - G.Juyn                                * */
/* *             - added tracecode for RGB8_A8 canvasstyle                  * */
/* *             0.5.2 - 06/06/2000 - G.Juyn                                * */
/* *             - added tracecode for mng_read_resume HLAPI function       * */
/* *                                                                        * */
/* *             0.5.3 - 06/06/2000 - G.Juyn                                * */
/* *             - added tracecodes for tracing JPEG progression            * */
/* *             0.5.3 - 06/21/2000 - G.Juyn                                * */
/* *             - added tracecodes for get/set speedtype                   * */
/* *             - added tracecodes for get imagelevel                      * */
/* *             0.5.3 - 06/22/2000 - G.Juyn                                * */
/* *             - added tracecode for delta-image processing               * */
/* *             - added tracecodes for PPLT chunk processing               * */
/* *                                                                        * */
/* ************************************************************************** */

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

#ifndef _mng_trace_h_
#define _mng_trace_h_

/* ************************************************************************** */

#ifdef MNG_INCLUDE_TRACE_PROCS

/* ************************************************************************** */

/* TODO: add a trace-mask so certain functions can be excluded */

mng_retcode mng_trace (mng_datap  pData,
                       mng_uint32 iFunction,
                       mng_uint32 iLocation);

/* ************************************************************************** */

#define MNG_TRACE(D,F,L)  { mng_retcode iR = mng_trace (D,F,L); \
                            if (iR) return iR; }

#define MNG_TRACEB(D,F,L) { if (mng_trace (D,F,L)) return MNG_FALSE; }

#define MNG_TRACEX(D,F,L) { if (mng_trace (D,F,L)) return 0; }

/* ************************************************************************** */

#define MNG_LC_START                    1
#define MNG_LC_END                      2
#define MNG_LC_INITIALIZE               3
#define MNG_LC_CLEANUP                  4

/* ************************************************************************** */

#define MNG_LC_JPEG_CREATE_DECOMPRESS   101
#define MNG_LC_JPEG_READ_HEADER         102
#define MNG_LC_JPEG_START_DECOMPRESS    103
#define MNG_LC_JPEG_START_OUTPUT        104
#define MNG_LC_JPEG_READ_SCANLINES      105
#define MNG_LC_JPEG_FINISH_OUTPUT       106
#define MNG_LC_JPEG_FINISH_DECOMPRESS   107
#define MNG_LC_JPEG_DESTROY_DECOMPRESS  108

/* ************************************************************************** */

#define MNG_FN_INITIALIZE               1
#define MNG_FN_RESET                    2
#define MNG_FN_CLEANUP                  3
#define MNG_FN_READ                     4
#define MNG_FN_WRITE                    5
#define MNG_FN_CREATE                   6
#define MNG_FN_READDISPLAY              7
#define MNG_FN_DISPLAY                  8
#define MNG_FN_DISPLAY_RESUME           9
#define MNG_FN_DISPLAY_FREEZE          10
#define MNG_FN_DISPLAY_RESET           11
#define MNG_FN_DISPLAY_GOFRAME         12
#define MNG_FN_DISPLAY_GOLAYER         13
#define MNG_FN_DISPLAY_GOTIME          14
#define MNG_FN_GETLASTERROR            15
#define MNG_FN_READ_RESUME             16

#define MNG_FN_SETCB_MEMALLOC         101
#define MNG_FN_SETCB_MEMFREE          102
#define MNG_FN_SETCB_READDATA         103
#define MNG_FN_SETCB_WRITEDATA        104
#define MNG_FN_SETCB_ERRORPROC        105
#define MNG_FN_SETCB_TRACEPROC        106
#define MNG_FN_SETCB_PROCESSHEADER    107
#define MNG_FN_SETCB_PROCESSTEXT      108
#define MNG_FN_SETCB_GETCANVASLINE    109
#define MNG_FN_SETCB_GETBKGDLINE      110
#define MNG_FN_SETCB_REFRESH          111
#define MNG_FN_SETCB_GETTICKCOUNT     112
#define MNG_FN_SETCB_SETTIMER         113
#define MNG_FN_SETCB_PROCESSGAMMA     114
#define MNG_FN_SETCB_PROCESSCHROMA    115
#define MNG_FN_SETCB_PROCESSSRGB      116
#define MNG_FN_SETCB_PROCESSICCP      117
#define MNG_FN_SETCB_PROCESSAROW      118
#define MNG_FN_SETCB_OPENSTREAM       119
#define MNG_FN_SETCB_CLOSESTREAM      120
#define MNG_FN_SETCB_GETALPHALINE     121

#define MNG_FN_GETCB_MEMALLOC         201
#define MNG_FN_GETCB_MEMFREE          202
#define MNG_FN_GETCB_READDATA         203
#define MNG_FN_GETCB_WRITEDATA        204
#define MNG_FN_GETCB_ERRORPROC        205
#define MNG_FN_GETCB_TRACEPROC        206
#define MNG_FN_GETCB_PROCESSHEADER    207
#define MNG_FN_GETCB_PROCESSTEXT      208
#define MNG_FN_GETCB_GETCANVASLINE    209
#define MNG_FN_GETCB_GETBKGDLINE      210
#define MNG_FN_GETCB_REFRESH          211
#define MNG_FN_GETCB_GETTICKCOUNT     212
#define MNG_FN_GETCB_SETTIMER         213
#define MNG_FN_GETCB_PROCESSGAMMA     214
#define MNG_FN_GETCB_PROCESSCHROMA    215
#define MNG_FN_GETCB_PROCESSSRGB      216
#define MNG_FN_GETCB_PROCESSICCP      217
#define MNG_FN_GETCB_PROCESSAROW      218
#define MNG_FN_GETCB_OPENSTREAM       219
#define MNG_FN_GETCB_CLOSESTREAM      220
#define MNG_FN_GETCB_GETALPHALINE     221

#define MNG_FN_SET_USERDATA           301
#define MNG_FN_SET_CANVASSTYLE        302
#define MNG_FN_SET_BKGDSTYLE          303
#define MNG_FN_SET_BGCOLOR            304
#define MNG_FN_SET_STORECHUNKS        305
#define MNG_FN_SET_VIEWGAMMA          306
#define MNG_FN_SET_DISPLAYGAMMA       307
#define MNG_FN_SET_DFLTIMGGAMMA       308
#define MNG_FN_SET_SRGB               309
#define MNG_FN_SET_OUTPUTPROFILE      310
#define MNG_FN_SET_SRGBPROFILE        311
#define MNG_FN_SET_MAXCANVASWIDTH     312
#define MNG_FN_SET_MAXCANVASHEIGHT    313
#define MNG_FN_SET_MAXCANVASSIZE      314
#define MNG_FN_SET_ZLIB_LEVEL         315
#define MNG_FN_SET_ZLIB_METHOD        316
#define MNG_FN_SET_ZLIB_WINDOWBITS    317
#define MNG_FN_SET_ZLIB_MEMLEVEL      318
#define MNG_FN_SET_ZLIB_STRATEGY      319
#define MNG_FN_SET_ZLIB_MAXIDAT       320
#define MNG_FN_SET_JPEG_DCTMETHOD     321
#define MNG_FN_SET_JPEG_QUALITY       322
#define MNG_FN_SET_JPEG_SMOOTHING     323
#define MNG_FN_SET_JPEG_PROGRESSIVE   324
#define MNG_FN_SET_JPEG_OPTIMIZED     325
#define MNG_FN_SET_JPEG_MAXJDAT       326
#define MNG_FN_SET_SPEED              327

#define MNG_FN_GET_USERDATA           401
#define MNG_FN_GET_SIGTYPE            402
#define MNG_FN_GET_IMAGETYPE          403
#define MNG_FN_GET_IMAGEWIDTH         404
#define MNG_FN_GET_IMAGEHEIGHT        405
#define MNG_FN_GET_TICKS              406
#define MNG_FN_GET_FRAMECOUNT         407
#define MNG_FN_GET_LAYERCOUNT         408
#define MNG_FN_GET_PLAYTIME           409
#define MNG_FN_GET_SIMPLICITY         410
#define MNG_FN_GET_CANVASSTYLE        411
#define MNG_FN_GET_BKGDSTYLE          412
#define MNG_FN_GET_BGCOLOR            413
#define MNG_FN_GET_STORECHUNKS        414
#define MNG_FN_GET_VIEWGAMMA          415
#define MNG_FN_GET_DISPLAYGAMMA       416
#define MNG_FN_GET_DFLTIMGGAMMA       417
#define MNG_FN_GET_SRGB               418
#define MNG_FN_GET_MAXCANVASWIDTH     419
#define MNG_FN_GET_MAXCANVASHEIGHT    420
#define MNG_FN_GET_ZLIB_LEVEL         421
#define MNG_FN_GET_ZLIB_METHOD        422
#define MNG_FN_GET_ZLIB_WINDOWBITS    423
#define MNG_FN_GET_ZLIB_MEMLEVEL      424
#define MNG_FN_GET_ZLIB_STRATEGY      425
#define MNG_FN_GET_ZLIB_MAXIDAT       426
#define MNG_FN_GET_JPEG_DCTMETHOD     427
#define MNG_FN_GET_JPEG_QUALITY       428
#define MNG_FN_GET_JPEG_SMOOTHING     429
#define MNG_FN_GET_JPEG_PROGRESSIVE   430
#define MNG_FN_GET_JPEG_OPTIMIZED     431
#define MNG_FN_GET_JPEG_MAXJDAT       432
#define MNG_FN_GET_SPEED              433
#define MNG_FN_GET_IMAGELEVEL         434

/* ************************************************************************** */

#define MNG_FN_ITERATE_CHUNKS         601

#define MNG_FN_GETCHUNK_IHDR          701
#define MNG_FN_GETCHUNK_PLTE          702
#define MNG_FN_GETCHUNK_IDAT          703
#define MNG_FN_GETCHUNK_IEND          704
#define MNG_FN_GETCHUNK_TRNS          705
#define MNG_FN_GETCHUNK_GAMA          706
#define MNG_FN_GETCHUNK_CHRM          707
#define MNG_FN_GETCHUNK_SRGB          708
#define MNG_FN_GETCHUNK_ICCP          709
#define MNG_FN_GETCHUNK_TEXT          710
#define MNG_FN_GETCHUNK_ZTXT          711
#define MNG_FN_GETCHUNK_ITXT          712
#define MNG_FN_GETCHUNK_BKGD          713
#define MNG_FN_GETCHUNK_PHYS          714
#define MNG_FN_GETCHUNK_SBIT          715
#define MNG_FN_GETCHUNK_SPLT          716
#define MNG_FN_GETCHUNK_HIST          717
#define MNG_FN_GETCHUNK_TIME          718
#define MNG_FN_GETCHUNK_MHDR          719
#define MNG_FN_GETCHUNK_MEND          720
#define MNG_FN_GETCHUNK_LOOP          721
#define MNG_FN_GETCHUNK_ENDL          722
#define MNG_FN_GETCHUNK_DEFI          723
#define MNG_FN_GETCHUNK_BASI          724
#define MNG_FN_GETCHUNK_CLON          725
#define MNG_FN_GETCHUNK_PAST          726
#define MNG_FN_GETCHUNK_DISC          727
#define MNG_FN_GETCHUNK_BACK          728
#define MNG_FN_GETCHUNK_FRAM          729
#define MNG_FN_GETCHUNK_MOVE          730
#define MNG_FN_GETCHUNK_CLIP          731
#define MNG_FN_GETCHUNK_SHOW          732
#define MNG_FN_GETCHUNK_TERM          733
#define MNG_FN_GETCHUNK_SAVE          734
#define MNG_FN_GETCHUNK_SEEK          735
#define MNG_FN_GETCHUNK_EXPI          736
#define MNG_FN_GETCHUNK_FPRI          737
#define MNG_FN_GETCHUNK_NEED          738
#define MNG_FN_GETCHUNK_PHYG          739
#define MNG_FN_GETCHUNK_JHDR          740
#define MNG_FN_GETCHUNK_JDAT          741
#define MNG_FN_GETCHUNK_JSEP          742
#define MNG_FN_GETCHUNK_DHDR          743
#define MNG_FN_GETCHUNK_PROM          744
#define MNG_FN_GETCHUNK_IPNG          745
#define MNG_FN_GETCHUNK_PPLT          746
#define MNG_FN_GETCHUNK_IJNG          747
#define MNG_FN_GETCHUNK_DROP          748
#define MNG_FN_GETCHUNK_DBYK          749
#define MNG_FN_GETCHUNK_ORDR          750
#define MNG_FN_GETCHUNK_UNKNOWN       751

#define MNG_FN_GETCHUNK_PAST_SRC      781
#define MNG_FN_GETCHUNK_SAVE_ENTRY    782
#define MNG_FN_GETCHUNK_PPLT_ENTRY    783
#define MNG_FN_GETCHUNK_ORDR_ENTRY    784

#define MNG_FN_PUTCHUNK_IHDR          801
#define MNG_FN_PUTCHUNK_PLTE          802
#define MNG_FN_PUTCHUNK_IDAT          803
#define MNG_FN_PUTCHUNK_IEND          804
#define MNG_FN_PUTCHUNK_TRNS          805
#define MNG_FN_PUTCHUNK_GAMA          806
#define MNG_FN_PUTCHUNK_CHRM          807
#define MNG_FN_PUTCHUNK_SRGB          808
#define MNG_FN_PUTCHUNK_ICCP          809
#define MNG_FN_PUTCHUNK_TEXT          810
#define MNG_FN_PUTCHUNK_ZTXT          811
#define MNG_FN_PUTCHUNK_ITXT          812
#define MNG_FN_PUTCHUNK_BKGD          813
#define MNG_FN_PUTCHUNK_PHYS          814
#define MNG_FN_PUTCHUNK_SBIT          815
#define MNG_FN_PUTCHUNK_SPLT          816
#define MNG_FN_PUTCHUNK_HIST          817
#define MNG_FN_PUTCHUNK_TIME          818
#define MNG_FN_PUTCHUNK_MHDR          819
#define MNG_FN_PUTCHUNK_MEND          820
#define MNG_FN_PUTCHUNK_LOOP          821
#define MNG_FN_PUTCHUNK_ENDL          822
#define MNG_FN_PUTCHUNK_DEFI          823
#define MNG_FN_PUTCHUNK_BASI          824
#define MNG_FN_PUTCHUNK_CLON          825
#define MNG_FN_PUTCHUNK_PAST          826
#define MNG_FN_PUTCHUNK_DISC          827
#define MNG_FN_PUTCHUNK_BACK          828
#define MNG_FN_PUTCHUNK_FRAM          829
#define MNG_FN_PUTCHUNK_MOVE          830
#define MNG_FN_PUTCHUNK_CLIP          831
#define MNG_FN_PUTCHUNK_SHOW          832
#define MNG_FN_PUTCHUNK_TERM          833
#define MNG_FN_PUTCHUNK_SAVE          834
#define MNG_FN_PUTCHUNK_SEEK          835
#define MNG_FN_PUTCHUNK_EXPI          836
#define MNG_FN_PUTCHUNK_FPRI          837
#define MNG_FN_PUTCHUNK_NEED          838
#define MNG_FN_PUTCHUNK_PHYG          839
#define MNG_FN_PUTCHUNK_JHDR          840
#define MNG_FN_PUTCHUNK_JDAT          841
#define MNG_FN_PUTCHUNK_JSEP          842
#define MNG_FN_PUTCHUNK_DHDR          843
#define MNG_FN_PUTCHUNK_PROM          844
#define MNG_FN_PUTCHUNK_IPNG          845
#define MNG_FN_PUTCHUNK_PPLT          846
#define MNG_FN_PUTCHUNK_IJNG          847
#define MNG_FN_PUTCHUNK_DROP          848
#define MNG_FN_PUTCHUNK_DBYK          849
#define MNG_FN_PUTCHUNK_ORDR          850
#define MNG_FN_PUTCHUNK_UNKNOWN       851

#define MNG_FN_PUTCHUNK_PAST_SRC      881
#define MNG_FN_PUTCHUNK_SAVE_ENTRY    882
#define MNG_FN_PUTCHUNK_PPLT_ENTRY    883
#define MNG_FN_PUTCHUNK_ORDR_ENTRY    884

/* ************************************************************************** */

#define MNG_FN_GETIMGDATA_SEQ         901
#define MNG_FN_GETIMGDATA_CHUNKSEQ    902
#define MNG_FN_GETIMGDATA_CHUNK       903

#define MNG_FN_PUTIMGDATA_IHDR        951
#define MNG_FN_PUTIMGDATA_JHDR        952
#define MNG_FN_PUTIMGDATA_BASI        953
#define MNG_FN_PUTIMGDATA_DHDR        954

/* ************************************************************************** */

#define MNG_FN_PROCESS_RAW_CHUNK     1001
#define MNG_FN_READ_GRAPHIC          1002
#define MNG_FN_DROP_CHUNKS           1003
#define MNG_FN_PROCESS_ERROR         1004
#define MNG_FN_CLEAR_CMS             1005
#define MNG_FN_DROP_OBJECTS          1006
#define MNG_FN_READ_CHUNK            1007
#define MNG_FN_LOAD_BKGDLAYER        1008
#define MNG_FN_NEXT_FRAME            1009
#define MNG_FN_NEXT_LAYER            1010
#define MNG_FN_INTERFRAME_DELAY      1011
#define MNG_FN_DISPLAY_IMAGE         1012
#define MNG_FN_DROP_IMGOBJECTS       1013
#define MNG_FN_DROP_ANIOBJECTS       1014
#define MNG_FN_INFLATE_BUFFER        1015
#define MNG_FN_DEFLATE_BUFFER        1016
#define MNG_FN_WRITE_RAW_CHUNK       1017
#define MNG_FN_WRITE_GRAPHIC         1018
#define MNG_FN_SAVE_STATE            1019
#define MNG_FN_RESTORE_STATE         1020
#define MNG_FN_DROP_SAVEDATA         1021
#define MNG_FN_EXECUTE_DELTA_IMAGE   1022

/* ************************************************************************** */

#define MNG_FN_DISPLAY_RGB8          1101
#define MNG_FN_DISPLAY_RGBA8         1102
#define MNG_FN_DISPLAY_ARGB8         1103
#define MNG_FN_DISPLAY_BGR8          1104
#define MNG_FN_DISPLAY_BGRA8         1105
#define MNG_FN_DISPLAY_ABGR8         1106
#define MNG_FN_DISPLAY_RGB16         1107
#define MNG_FN_DISPLAY_RGBA16        1108
#define MNG_FN_DISPLAY_ARGB16        1109
#define MNG_FN_DISPLAY_BGR16         1110
#define MNG_FN_DISPLAY_BGRA16        1111
#define MNG_FN_DISPLAY_ABGR16        1112
#define MNG_FN_DISPLAY_INDEX8        1113
#define MNG_FN_DISPLAY_INDEXA8       1114
#define MNG_FN_DISPLAY_AINDEX8       1115
#define MNG_FN_DISPLAY_GRAY8         1116
#define MNG_FN_DISPLAY_GRAY16        1117
#define MNG_FN_DISPLAY_GRAYA8        1118
#define MNG_FN_DISPLAY_GRAYA16       1119
#define MNG_FN_DISPLAY_AGRAY8        1120
#define MNG_FN_DISPLAY_AGRAY16       1121
#define MNG_FN_DISPLAY_DX15          1122
#define MNG_FN_DISPLAY_DX16          1123
#define MNG_FN_DISPLAY_RGB8_A8       1124

/* ************************************************************************** */

#define MNG_FN_INIT_FULL_CMS         1201
#define MNG_FN_CORRECT_FULL_CMS      1202
#define MNG_FN_INIT_GAMMA_ONLY       1204
#define MNG_FN_CORRECT_GAMMA_ONLY    1205
#define MNG_FN_CORRECT_APP_CMS       1206
#define MNG_FN_INIT_FULL_CMS_OBJ     1207
#define MNG_FN_INIT_GAMMA_ONLY_OBJ   1208
#define MNG_FN_INIT_APP_CMS          1209
#define MNG_FN_INIT_APP_CMS_OBJ      1210

/* ************************************************************************** */

#define MNG_FN_PROCESS_G1            1301
#define MNG_FN_PROCESS_G2            1302
#define MNG_FN_PROCESS_G4            1303
#define MNG_FN_PROCESS_G8            1304
#define MNG_FN_PROCESS_G16           1305
#define MNG_FN_PROCESS_RGB8          1306
#define MNG_FN_PROCESS_RGB16         1307
#define MNG_FN_PROCESS_IDX1          1308
#define MNG_FN_PROCESS_IDX2          1309
#define MNG_FN_PROCESS_IDX4          1310
#define MNG_FN_PROCESS_IDX8          1311
#define MNG_FN_PROCESS_GA8           1312
#define MNG_FN_PROCESS_GA16          1313
#define MNG_FN_PROCESS_RGBA8         1314
#define MNG_FN_PROCESS_RGBA16        1315

/* ************************************************************************** */

#define MNG_FN_INIT_G1_NI            1401
#define MNG_FN_INIT_G1_I             1402
#define MNG_FN_INIT_G2_NI            1403
#define MNG_FN_INIT_G2_I             1404
#define MNG_FN_INIT_G4_NI            1405
#define MNG_FN_INIT_G4_I             1406
#define MNG_FN_INIT_G8_NI            1407
#define MNG_FN_INIT_G8_I             1408
#define MNG_FN_INIT_G16_NI           1409
#define MNG_FN_INIT_G16_I            1410
#define MNG_FN_INIT_RGB8_NI          1411
#define MNG_FN_INIT_RGB8_I           1412
#define MNG_FN_INIT_RGB16_NI         1413
#define MNG_FN_INIT_RGB16_I          1414
#define MNG_FN_INIT_IDX1_NI          1415
#define MNG_FN_INIT_IDX1_I           1416
#define MNG_FN_INIT_IDX2_NI          1417
#define MNG_FN_INIT_IDX2_I           1418
#define MNG_FN_INIT_IDX4_NI          1419
#define MNG_FN_INIT_IDX4_I           1420
#define MNG_FN_INIT_IDX8_NI          1421
#define MNG_FN_INIT_IDX8_I           1422
#define MNG_FN_INIT_GA8_NI           1423
#define MNG_FN_INIT_GA8_I            1424
#define MNG_FN_INIT_GA16_NI          1425
#define MNG_FN_INIT_GA16_I           1426
#define MNG_FN_INIT_RGBA8_NI         1427
#define MNG_FN_INIT_RGBA8_I          1428
#define MNG_FN_INIT_RGBA16_NI        1429
#define MNG_FN_INIT_RGBA16_I         1430

#define MNG_FN_INIT_ROWPROC          1497
#define MNG_FN_NEXT_ROW              1498
#define MNG_FN_CLEANUP_ROWPROC       1499

/* ************************************************************************** */

#define MNG_FN_FILTER_A_ROW          1501
#define MNG_FN_FILTER_SUB            1502
#define MNG_FN_FILTER_UP             1503
#define MNG_FN_FILTER_AVERAGE        1504
#define MNG_FN_FILTER_PAETH          1505

/* ************************************************************************** */

#define MNG_FN_CREATE_IMGDATAOBJECT  1601
#define MNG_FN_FREE_IMGDATAOBJECT    1602
#define MNG_FN_CLONE_IMGDATAOBJECT   1603
#define MNG_FN_CREATE_IMGOBJECT      1604
#define MNG_FN_FREE_IMGOBJECT        1605
#define MNG_FN_FIND_IMGOBJECT        1606
#define MNG_FN_CLONE_IMGOBJECT       1607
#define MNG_FN_RESET_OBJECTDETAILS   1608
#define MNG_FN_RENUM_IMGOBJECT       1609
#define MNG_FN_PROMOTE_IMGOBJECT     1610

/* ************************************************************************** */

#define MNG_FN_STORE_G1              1701
#define MNG_FN_STORE_G2              1702
#define MNG_FN_STORE_G4              1703
#define MNG_FN_STORE_G8              1704
#define MNG_FN_STORE_G16             1705
#define MNG_FN_STORE_RGB8            1706
#define MNG_FN_STORE_RGB16           1707
#define MNG_FN_STORE_IDX1            1708
#define MNG_FN_STORE_IDX2            1709
#define MNG_FN_STORE_IDX4            1710
#define MNG_FN_STORE_IDX8            1711
#define MNG_FN_STORE_GA8             1712
#define MNG_FN_STORE_GA16            1713
#define MNG_FN_STORE_RGBA8           1714
#define MNG_FN_STORE_RGBA16          1715

#define MNG_FN_RETRIEVE_G8           1751
#define MNG_FN_RETRIEVE_G16          1752
#define MNG_FN_RETRIEVE_RGB8         1753
#define MNG_FN_RETRIEVE_RGB16        1754
#define MNG_FN_RETRIEVE_IDX8         1755
#define MNG_FN_RETRIEVE_GA8          1756
#define MNG_FN_RETRIEVE_GA16         1757
#define MNG_FN_RETRIEVE_RGBA8        1758
#define MNG_FN_RETRIEVE_RGBA16       1759

#define MNG_FN_DELTA_G1              1771
#define MNG_FN_DELTA_G2              1772
#define MNG_FN_DELTA_G4              1773
#define MNG_FN_DELTA_G8              1774
#define MNG_FN_DELTA_G16             1775
#define MNG_FN_DELTA_RGB8            1776
#define MNG_FN_DELTA_RGB16           1777
#define MNG_FN_DELTA_IDX1            1778
#define MNG_FN_DELTA_IDX2            1779
#define MNG_FN_DELTA_IDX4            1780
#define MNG_FN_DELTA_IDX8            1781
#define MNG_FN_DELTA_GA8             1782
#define MNG_FN_DELTA_GA16            1783
#define MNG_FN_DELTA_RGBA8           1784
#define MNG_FN_DELTA_RGBA16          1785

/* ************************************************************************** */

#define MNG_FN_CREATE_ANI_LOOP       1801
#define MNG_FN_CREATE_ANI_ENDL       1802
#define MNG_FN_CREATE_ANI_DEFI       1803
#define MNG_FN_CREATE_ANI_BASI       1804
#define MNG_FN_CREATE_ANI_CLON       1805
#define MNG_FN_CREATE_ANI_PAST       1806
#define MNG_FN_CREATE_ANI_DISC       1807
#define MNG_FN_CREATE_ANI_BACK       1808
#define MNG_FN_CREATE_ANI_FRAM       1809
#define MNG_FN_CREATE_ANI_MOVE       1810
#define MNG_FN_CREATE_ANI_CLIP       1811
#define MNG_FN_CREATE_ANI_SHOW       1812
#define MNG_FN_CREATE_ANI_TERM       1813
#define MNG_FN_CREATE_ANI_SAVE       1814
#define MNG_FN_CREATE_ANI_SEEK       1815
#define MNG_FN_CREATE_ANI_GAMA       1816
#define MNG_FN_CREATE_ANI_CHRM       1817
#define MNG_FN_CREATE_ANI_SRGB       1818
#define MNG_FN_CREATE_ANI_ICCP       1819
#define MNG_FN_CREATE_ANI_PLTE       1820
#define MNG_FN_CREATE_ANI_TRNS       1821
#define MNG_FN_CREATE_ANI_BKGD       1822
#define MNG_FN_CREATE_ANI_DHDR       1823
#define MNG_FN_CREATE_ANI_PROM       1824
#define MNG_FN_CREATE_ANI_IPNG       1825
#define MNG_FN_CREATE_ANI_IJNG       1826
#define MNG_FN_CREATE_ANI_PPLT       1827

#define MNG_FN_CREATE_ANI_IMAGE      1891

/* ************************************************************************** */

#define MNG_FN_FREE_ANI_LOOP         1901
#define MNG_FN_FREE_ANI_ENDL         1902
#define MNG_FN_FREE_ANI_DEFI         1903
#define MNG_FN_FREE_ANI_BASI         1904
#define MNG_FN_FREE_ANI_CLON         1905
#define MNG_FN_FREE_ANI_PAST         1906
#define MNG_FN_FREE_ANI_DISC         1907
#define MNG_FN_FREE_ANI_BACK         1908
#define MNG_FN_FREE_ANI_FRAM         1909
#define MNG_FN_FREE_ANI_MOVE         1910
#define MNG_FN_FREE_ANI_CLIP         1911
#define MNG_FN_FREE_ANI_SHOW         1912
#define MNG_FN_FREE_ANI_TERM         1913
#define MNG_FN_FREE_ANI_SAVE         1914
#define MNG_FN_FREE_ANI_SEEK         1915
#define MNG_FN_FREE_ANI_GAMA         1916
#define MNG_FN_FREE_ANI_CHRM         1917
#define MNG_FN_FREE_ANI_SRGB         1918
#define MNG_FN_FREE_ANI_ICCP         1919
#define MNG_FN_FREE_ANI_PLTE         1920
#define MNG_FN_FREE_ANI_TRNS         1921
#define MNG_FN_FREE_ANI_BKGD         1922
#define MNG_FN_FREE_ANI_DHDR         1923
#define MNG_FN_FREE_ANI_PROM         1924
#define MNG_FN_FREE_ANI_IPNG         1925
#define MNG_FN_FREE_ANI_IJNG         1926
#define MNG_FN_FREE_ANI_PPLT         1927

#define MNG_FN_FREE_ANI_IMAGE        1991

/* ************************************************************************** */

#define MNG_FN_PROCESS_ANI_LOOP      2001
#define MNG_FN_PROCESS_ANI_ENDL      2002
#define MNG_FN_PROCESS_ANI_DEFI      2003
#define MNG_FN_PROCESS_ANI_BASI      2004
#define MNG_FN_PROCESS_ANI_CLON      2005
#define MNG_FN_PROCESS_ANI_PAST      2006
#define MNG_FN_PROCESS_ANI_DISC      2007
#define MNG_FN_PROCESS_ANI_BACK      2008
#define MNG_FN_PROCESS_ANI_FRAM      2009
#define MNG_FN_PROCESS_ANI_MOVE      2010
#define MNG_FN_PROCESS_ANI_CLIP      2011
#define MNG_FN_PROCESS_ANI_SHOW      2012
#define MNG_FN_PROCESS_ANI_TERM      2013
#define MNG_FN_PROCESS_ANI_SAVE      2014
#define MNG_FN_PROCESS_ANI_SEEK      2015
#define MNG_FN_PROCESS_ANI_GAMA      2016
#define MNG_FN_PROCESS_ANI_CHRM      2017
#define MNG_FN_PROCESS_ANI_SRGB      2018
#define MNG_FN_PROCESS_ANI_ICCP      2019
#define MNG_FN_PROCESS_ANI_PLTE      2020
#define MNG_FN_PROCESS_ANI_TRNS      2021
#define MNG_FN_PROCESS_ANI_BKGD      2022
#define MNG_FN_PROCESS_ANI_DHDR      2023
#define MNG_FN_PROCESS_ANI_PROM      2024
#define MNG_FN_PROCESS_ANI_IPNG      2025
#define MNG_FN_PROCESS_ANI_IJNG      2026
#define MNG_FN_PROCESS_ANI_PPLT      2027

#define MNG_FN_PROCESS_ANI_IMAGE     2091

/* ************************************************************************** */

#define MNG_FN_RESTORE_BACKIMAGE     2101
#define MNG_FN_RESTORE_BACKCOLOR     2102
#define MNG_FN_RESTORE_BGCOLOR       2103
#define MNG_FN_RESTORE_RGB8          2104
#define MNG_FN_RESTORE_BGR8          2105

/* ************************************************************************** */

#define MNG_FN_INIT_IHDR             2201
#define MNG_FN_INIT_PLTE             2202
#define MNG_FN_INIT_IDAT             2203
#define MNG_FN_INIT_IEND             2204
#define MNG_FN_INIT_TRNS             2205
#define MNG_FN_INIT_GAMA             2206
#define MNG_FN_INIT_CHRM             2207
#define MNG_FN_INIT_SRGB             2208
#define MNG_FN_INIT_ICCP             2209
#define MNG_FN_INIT_TEXT             2210
#define MNG_FN_INIT_ZTXT             2211
#define MNG_FN_INIT_ITXT             2212
#define MNG_FN_INIT_BKGD             2213
#define MNG_FN_INIT_PHYS             2214
#define MNG_FN_INIT_SBIT             2215
#define MNG_FN_INIT_SPLT             2216
#define MNG_FN_INIT_HIST             2217
#define MNG_FN_INIT_TIME             2218
#define MNG_FN_INIT_MHDR             2219
#define MNG_FN_INIT_MEND             2220
#define MNG_FN_INIT_LOOP             2221
#define MNG_FN_INIT_ENDL             2222
#define MNG_FN_INIT_DEFI             2223
#define MNG_FN_INIT_BASI             2224
#define MNG_FN_INIT_CLON             2225
#define MNG_FN_INIT_PAST             2226
#define MNG_FN_INIT_DISC             2227
#define MNG_FN_INIT_BACK             2228
#define MNG_FN_INIT_FRAM             2229
#define MNG_FN_INIT_MOVE             2230
#define MNG_FN_INIT_CLIP             2231
#define MNG_FN_INIT_SHOW             2232
#define MNG_FN_INIT_TERM             2233
#define MNG_FN_INIT_SAVE             2234
#define MNG_FN_INIT_SEEK             2235
#define MNG_FN_INIT_EXPI             2236
#define MNG_FN_INIT_FPRI             2237
#define MNG_FN_INIT_NEED             2238
#define MNG_FN_INIT_PHYG             2239
#define MNG_FN_INIT_JHDR             2240
#define MNG_FN_INIT_JDAT             2241
#define MNG_FN_INIT_JSEP             2242
#define MNG_FN_INIT_DHDR             2243
#define MNG_FN_INIT_PROM             2244
#define MNG_FN_INIT_IPNG             2245
#define MNG_FN_INIT_PPLT             2246
#define MNG_FN_INIT_IJNG             2247
#define MNG_FN_INIT_DROP             2248
#define MNG_FN_INIT_DBYK             2249
#define MNG_FN_INIT_ORDR             2250
#define MNG_FN_INIT_UNKNOWN          2251

/* ************************************************************************** */

#define MNG_FN_FREE_IHDR             2401
#define MNG_FN_FREE_PLTE             2402
#define MNG_FN_FREE_IDAT             2403
#define MNG_FN_FREE_IEND             2404
#define MNG_FN_FREE_TRNS             2405
#define MNG_FN_FREE_GAMA             2406
#define MNG_FN_FREE_CHRM             2407
#define MNG_FN_FREE_SRGB             2408
#define MNG_FN_FREE_ICCP             2409
#define MNG_FN_FREE_TEXT             2410
#define MNG_FN_FREE_ZTXT             2411
#define MNG_FN_FREE_ITXT             2412
#define MNG_FN_FREE_BKGD             2413
#define MNG_FN_FREE_PHYS             2414
#define MNG_FN_FREE_SBIT             2415
#define MNG_FN_FREE_SPLT             2416
#define MNG_FN_FREE_HIST             2417
#define MNG_FN_FREE_TIME             2418
#define MNG_FN_FREE_MHDR             2419
#define MNG_FN_FREE_MEND             2420
#define MNG_FN_FREE_LOOP             2421
#define MNG_FN_FREE_ENDL             2422
#define MNG_FN_FREE_DEFI             2423
#define MNG_FN_FREE_BASI             2424
#define MNG_FN_FREE_CLON             2425
#define MNG_FN_FREE_PAST             2426
#define MNG_FN_FREE_DISC             2427
#define MNG_FN_FREE_BACK             2428
#define MNG_FN_FREE_FRAM             2429
#define MNG_FN_FREE_MOVE             2430
#define MNG_FN_FREE_CLIP             2431
#define MNG_FN_FREE_SHOW             2432
#define MNG_FN_FREE_TERM             2433
#define MNG_FN_FREE_SAVE             2434
#define MNG_FN_FREE_SEEK             2435
#define MNG_FN_FREE_EXPI             2436
#define MNG_FN_FREE_FPRI             2437
#define MNG_FN_FREE_NEED             2438
#define MNG_FN_FREE_PHYG             2439
#define MNG_FN_FREE_JHDR             2440
#define MNG_FN_FREE_JDAT             2441
#define MNG_FN_FREE_JSEP             2442
#define MNG_FN_FREE_DHDR             2443
#define MNG_FN_FREE_PROM             2444
#define MNG_FN_FREE_IPNG             2445
#define MNG_FN_FREE_PPLT             2446
#define MNG_FN_FREE_IJNG             2447
#define MNG_FN_FREE_DROP             2448
#define MNG_FN_FREE_DBYK             2449
#define MNG_FN_FREE_ORDR             2450
#define MNG_FN_FREE_UNKNOWN          2451

/* ************************************************************************** */

#define MNG_FN_READ_IHDR             2601
#define MNG_FN_READ_PLTE             2602
#define MNG_FN_READ_IDAT             2603
#define MNG_FN_READ_IEND             2604
#define MNG_FN_READ_TRNS             2605
#define MNG_FN_READ_GAMA             2606
#define MNG_FN_READ_CHRM             2607
#define MNG_FN_READ_SRGB             2608
#define MNG_FN_READ_ICCP             2609
#define MNG_FN_READ_TEXT             2610
#define MNG_FN_READ_ZTXT             2611
#define MNG_FN_READ_ITXT             2612
#define MNG_FN_READ_BKGD             2613
#define MNG_FN_READ_PHYS             2614
#define MNG_FN_READ_SBIT             2615
#define MNG_FN_READ_SPLT             2616
#define MNG_FN_READ_HIST             2617
#define MNG_FN_READ_TIME             2618
#define MNG_FN_READ_MHDR             2619
#define MNG_FN_READ_MEND             2620
#define MNG_FN_READ_LOOP             2621
#define MNG_FN_READ_ENDL             2622
#define MNG_FN_READ_DEFI             2623
#define MNG_FN_READ_BASI             2624
#define MNG_FN_READ_CLON             2625
#define MNG_FN_READ_PAST             2626
#define MNG_FN_READ_DISC             2627
#define MNG_FN_READ_BACK             2628
#define MNG_FN_READ_FRAM             2629
#define MNG_FN_READ_MOVE             2630
#define MNG_FN_READ_CLIP             2631
#define MNG_FN_READ_SHOW             2632
#define MNG_FN_READ_TERM             2633
#define MNG_FN_READ_SAVE             2634
#define MNG_FN_READ_SEEK             2635
#define MNG_FN_READ_EXPI             2636
#define MNG_FN_READ_FPRI             2637
#define MNG_FN_READ_NEED             2638
#define MNG_FN_READ_PHYG             2639
#define MNG_FN_READ_JHDR             2640
#define MNG_FN_READ_JDAT             2641
#define MNG_FN_READ_JSEP             2642
#define MNG_FN_READ_DHDR             2643
#define MNG_FN_READ_PROM             2644
#define MNG_FN_READ_IPNG             2645
#define MNG_FN_READ_PPLT             2646
#define MNG_FN_READ_IJNG             2647
#define MNG_FN_READ_DROP             2648
#define MNG_FN_READ_DBYK             2649
#define MNG_FN_READ_ORDR             2650
#define MNG_FN_READ_UNKNOWN          2651

/* ************************************************************************** */

#define MNG_FN_WRITE_IHDR            2801
#define MNG_FN_WRITE_PLTE            2802
#define MNG_FN_WRITE_IDAT            2803
#define MNG_FN_WRITE_IEND            2804
#define MNG_FN_WRITE_TRNS            2805
#define MNG_FN_WRITE_GAMA            2806
#define MNG_FN_WRITE_CHRM            2807
#define MNG_FN_WRITE_SRGB            2808
#define MNG_FN_WRITE_ICCP            2809
#define MNG_FN_WRITE_TEXT            2810
#define MNG_FN_WRITE_ZTXT            2811
#define MNG_FN_WRITE_ITXT            2812
#define MNG_FN_WRITE_BKGD            2813
#define MNG_FN_WRITE_PHYS            2814
#define MNG_FN_WRITE_SBIT            2815
#define MNG_FN_WRITE_SPLT            2816
#define MNG_FN_WRITE_HIST            2817
#define MNG_FN_WRITE_TIME            2818
#define MNG_FN_WRITE_MHDR            2819
#define MNG_FN_WRITE_MEND            2820
#define MNG_FN_WRITE_LOOP            2821
#define MNG_FN_WRITE_ENDL            2822
#define MNG_FN_WRITE_DEFI            2823
#define MNG_FN_WRITE_BASI            2824
#define MNG_FN_WRITE_CLON            2825
#define MNG_FN_WRITE_PAST            2826
#define MNG_FN_WRITE_DISC            2827
#define MNG_FN_WRITE_BACK            2828
#define MNG_FN_WRITE_FRAM            2829
#define MNG_FN_WRITE_MOVE            2830
#define MNG_FN_WRITE_CLIP            2831
#define MNG_FN_WRITE_SHOW            2832
#define MNG_FN_WRITE_TERM            2833
#define MNG_FN_WRITE_SAVE            2834
#define MNG_FN_WRITE_SEEK            2835
#define MNG_FN_WRITE_EXPI            2836
#define MNG_FN_WRITE_FPRI            2837
#define MNG_FN_WRITE_NEED            2838
#define MNG_FN_WRITE_PHYG            2839
#define MNG_FN_WRITE_JHDR            2840
#define MNG_FN_WRITE_JDAT            2841
#define MNG_FN_WRITE_JSEP            2842
#define MNG_FN_WRITE_DHDR            2843
#define MNG_FN_WRITE_PROM            2844
#define MNG_FN_WRITE_IPNG            2845
#define MNG_FN_WRITE_PPLT            2846
#define MNG_FN_WRITE_IJNG            2847
#define MNG_FN_WRITE_DROP            2848
#define MNG_FN_WRITE_DBYK            2849
#define MNG_FN_WRITE_ORDR            2850
#define MNG_FN_WRITE_UNKNOWN         2851

/* ************************************************************************** */

#define MNG_FN_ZLIB_INITIALIZE       3001
#define MNG_FN_ZLIB_CLEANUP          3002
#define MNG_FN_ZLIB_INFLATEINIT      3003
#define MNG_FN_ZLIB_INFLATEROWS      3004
#define MNG_FN_ZLIB_INFLATEDATA      3005
#define MNG_FN_ZLIB_INFLATEFREE      3006
#define MNG_FN_ZLIB_DEFLATEINIT      3007
#define MNG_FN_ZLIB_DEFLATEROWS      3008
#define MNG_FN_ZLIB_DEFLATEDATA      3009
#define MNG_FN_ZLIB_DEFLATEFREE      3010

/* ************************************************************************** */

#define MNG_FN_PROCESS_DISPLAY_IHDR  3201
#define MNG_FN_PROCESS_DISPLAY_PLTE  3202
#define MNG_FN_PROCESS_DISPLAY_IDAT  3203
#define MNG_FN_PROCESS_DISPLAY_IEND  3204
#define MNG_FN_PROCESS_DISPLAY_TRNS  3205
#define MNG_FN_PROCESS_DISPLAY_GAMA  3206
#define MNG_FN_PROCESS_DISPLAY_CHRM  3207
#define MNG_FN_PROCESS_DISPLAY_SRGB  3208
#define MNG_FN_PROCESS_DISPLAY_ICCP  3209
#define MNG_FN_PROCESS_DISPLAY_BKGD  3210
#define MNG_FN_PROCESS_DISPLAY_PHYS  3211
#define MNG_FN_PROCESS_DISPLAY_SBIT  3212
#define MNG_FN_PROCESS_DISPLAY_SPLT  3213
#define MNG_FN_PROCESS_DISPLAY_HIST  3214
#define MNG_FN_PROCESS_DISPLAY_MHDR  3215
#define MNG_FN_PROCESS_DISPLAY_MEND  3216
#define MNG_FN_PROCESS_DISPLAY_LOOP  3217
#define MNG_FN_PROCESS_DISPLAY_ENDL  3218
#define MNG_FN_PROCESS_DISPLAY_DEFI  3219
#define MNG_FN_PROCESS_DISPLAY_BASI  3220
#define MNG_FN_PROCESS_DISPLAY_CLON  3221
#define MNG_FN_PROCESS_DISPLAY_PAST  3222
#define MNG_FN_PROCESS_DISPLAY_DISC  3223
#define MNG_FN_PROCESS_DISPLAY_BACK  3224
#define MNG_FN_PROCESS_DISPLAY_FRAM  3225
#define MNG_FN_PROCESS_DISPLAY_MOVE  3226
#define MNG_FN_PROCESS_DISPLAY_CLIP  3227
#define MNG_FN_PROCESS_DISPLAY_SHOW  3228
#define MNG_FN_PROCESS_DISPLAY_TERM  3229
#define MNG_FN_PROCESS_DISPLAY_SAVE  3230
#define MNG_FN_PROCESS_DISPLAY_SEEK  3231
#define MNG_FN_PROCESS_DISPLAY_EXPI  3232
#define MNG_FN_PROCESS_DISPLAY_FPRI  3233
#define MNG_FN_PROCESS_DISPLAY_NEED  3234
#define MNG_FN_PROCESS_DISPLAY_PHYG  3235
#define MNG_FN_PROCESS_DISPLAY_JHDR  3236
#define MNG_FN_PROCESS_DISPLAY_JDAT  3237
#define MNG_FN_PROCESS_DISPLAY_JSEP  3238
#define MNG_FN_PROCESS_DISPLAY_DHDR  3239
#define MNG_FN_PROCESS_DISPLAY_PROM  3240
#define MNG_FN_PROCESS_DISPLAY_IPNG  3241
#define MNG_FN_PROCESS_DISPLAY_PPLT  3242
#define MNG_FN_PROCESS_DISPLAY_IJNG  3243
#define MNG_FN_PROCESS_DISPLAY_DROP  3244
#define MNG_FN_PROCESS_DISPLAY_DBYK  3245
#define MNG_FN_PROCESS_DISPLAY_ORDR  3246

/* ************************************************************************** */

#define MNG_FN_JPEG_INITIALIZE       3401
#define MNG_FN_JPEG_CLEANUP          3402
#define MNG_FN_JPEG_DECOMPRESSINIT   3403
#define MNG_FN_JPEG_DECOMPRESSDATA   3404
#define MNG_FN_JPEG_DECOMPRESSFREE   3405

#define MNG_FN_STORE_JPEG_G8         3501
#define MNG_FN_STORE_JPEG_RGB8       3502
#define MNG_FN_STORE_JPEG_G12        3503
#define MNG_FN_STORE_JPEG_RGB12      3504
#define MNG_FN_STORE_JPEG_GA8        3505
#define MNG_FN_STORE_JPEG_RGBA8      3506
#define MNG_FN_STORE_JPEG_GA12       3507
#define MNG_FN_STORE_JPEG_RGBA12     3508

#define MNG_FN_INIT_JPEG_A1_NI       3511
#define MNG_FN_INIT_JPEG_A2_NI       3512
#define MNG_FN_INIT_JPEG_A4_NI       3513
#define MNG_FN_INIT_JPEG_A8_NI       3514
#define MNG_FN_INIT_JPEG_A16_NI      3515

#define MNG_FN_STORE_JPEG_G8_A1      3521
#define MNG_FN_STORE_JPEG_G8_A2      3522
#define MNG_FN_STORE_JPEG_G8_A4      3523
#define MNG_FN_STORE_JPEG_G8_A8      3524
#define MNG_FN_STORE_JPEG_G8_A16     3525

#define MNG_FN_STORE_JPEG_RGB8_A1    3531
#define MNG_FN_STORE_JPEG_RGB8_A2    3532
#define MNG_FN_STORE_JPEG_RGB8_A4    3533
#define MNG_FN_STORE_JPEG_RGB8_A8    3534
#define MNG_FN_STORE_JPEG_RGB8_A16   3535

#define MNG_FN_STORE_JPEG_G12_A1     3541
#define MNG_FN_STORE_JPEG_G12_A2     3542
#define MNG_FN_STORE_JPEG_G12_A4     3543
#define MNG_FN_STORE_JPEG_G12_A8     3544
#define MNG_FN_STORE_JPEG_G12_A16    3545

#define MNG_FN_STORE_JPEG_RGB12_A1   3551
#define MNG_FN_STORE_JPEG_RGB12_A2   3552
#define MNG_FN_STORE_JPEG_RGB12_A4   3553
#define MNG_FN_STORE_JPEG_RGB12_A8   3554
#define MNG_FN_STORE_JPEG_RGB12_A16  3555

#define MNG_FN_NEXT_JPEG_ALPHAROW    3591
#define MNG_FN_NEXT_JPEG_ROW         3592
#define MNG_FN_DISPLAY_JPEG_ROWS     3593

/* ************************************************************************** */
/* *                                                                        * */
/* * Trace string-table entry                                               * */
/* *                                                                        * */
/* ************************************************************************** */

typedef struct {
           mng_uint32 iFunction;
           mng_pchar  zTracetext;
        } mng_trace_entry;
typedef mng_trace_entry * mng_trace_entryp;

/* ************************************************************************** */

#endif /* MNG_INCLUDE_TRACE_PROCS */

/* ************************************************************************** */

#endif /* _mng_trace_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */

