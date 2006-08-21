#!/usr/bin/python

import cgitb; cgitb.enable()

import os
import sys
import cgi
import time
import re
import gzip
import minjson as json

import cStringIO

from pysqlite2 import dbapi2 as sqlite

DBPATH = "db/data.sqlite"

#
# All objects are returned in the form:
# {
#  resultcode: n,
#  ...
# }
#
# The ... is dependant on the result type.
#
# Result codes:
#   0 success
#  -1 bad tinderbox
#  -2 bad test name
#
# incoming query string:
# tbox=name
#  tinderbox name
#
# If only tbox specified, returns array of test names for that tinderbox in data
# If invalid tbox specified, returns error -1
#
# test=testname
#  test name
#
# Returns results for that test in .results, in array of [time0, value0, time1, value1, ...]
# Also reteurns .annotations, in array of [time0, string0, time1, string1, ...]
#
# raw=1
# Same as full results, but includes raw data for test, in form [time0, value0, rawresult0, ...]
#
# starttime=tval
#  Start time to return results from, in seconds since GMT epoch
# endtime=tval
#  End time, in seconds since GMT epoch
#

def doError(errCode):
    errString = "unknown error"
    if errCode == -1:
        errString = "bad tinderbox"
    elif errCode == -2:
        errString = "bad test name"
    print "{ resultcode: " + str(errCode) + ", error: '" + errString + "' }"

db = None

def doListTests(fo, tbox):
    results = []
    
    cur = db.cursor()
    cur.execute("SELECT id, machine, test, test_type, extra_data FROM dataset_info")
    for row in cur:
        results.append( {"id": row[0],
                         "machine": row[1],
                         "test": row[2],
                         "test_type": row[3],
                         "extra_data": row[4]} )
    cur.close()
    fo.write (json.write( {"resultcode": 0, "results": results} ))

def doSendResults(fo, testid, starttime, endtime, raw):
    raws = ""
    if raw is not None:
        raws = ", extra"
    s1 = ""
    s2 = ""
    if starttime is not None:
        s1 = " AND time >= " + starttime
    if endtime is not None:
        s2 = " AND time <= " + endtime

    fo.write ("{ resultcode: 0,")

    cur = db.cursor()
    cur.execute("SELECT time, value" + raws + " FROM datasets WHERE dataset_id = ? " + s1 + s2 + " ORDER BY time", (testid))
    fo.write ("results: [")
    for row in cur:
        if row[1] == 'nan':
            continue
        if raw:
            fo.write ("%s,%s,'%s'," % (row[0], row[1], row[2]))
        else:
            fo.write ("%s,%s," % (row[0], row[1]))
    cur.close()
    fo.write ("],")

    cur = db.cursor()
    cur.execute("SELECT time, value FROM annotations WHERE dataset_id = ? " + s1 + s2 + " ORDER BY time", (testid))
    fo.write ("annotations: [")
    for row in cur:
        fo.write("%s,'%s'," % (row[0], row[1]))
    cur.close()
    fo.write ("],")

    fo.write ("}")

def main():
    doGzip = 0
    try:
        if string.find(os.environ["HTTP_ACCEPT_ENCODING"], "gzip") != -1:
            doGzip = 1
    except:
        pass

    form = cgi.FieldStorage()

    setid = form.getfirst("setid")
    raw = form.getfirst("raw")
    starttime = form.getfirst("starttime")
    endtime = form.getfirst("endtime")

    zbuf = cStringIO.StringIO()
    zfile = zbuf
    if doGzip == 1:
        zfile = gzip.GzipFile(mode = 'wb', fileobj = zbuf, compresslevel = 5)

    global db
    db = sqlite.connect(dbfile)
    
    if setid is None:
        doListTests(zfile)
    else:
        doSendResults(zfile, setid, starttime, endtime, raw)

    sys.stdout.write("Content-Type: text/plain\n")
    if doGzip == 1:
        zfile.close()
        sys.stdout.write("Content-Encoding: gzip\n")
    sys.stdout.write("\n")

    sys.stdout.write(zbuf.getvalue())

main()


