/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
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
 * Copyright (C) 2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Ben Goodger <ben@netscape.com> (Original Author)
 */
 
const _DEBUG = true;
function _dd(aString)
{
  if (_DEBUG)
    dump("*** " + aString + "\n");
}

function _ddf(aString, aValue)
{
  if (_DEBUG)
    dump("*** " + aString + " = " + aValue + "\n");
}

var vxUtils = {
  getWindow: function (aWindowType)
  {
    const WM_CONTRACTID = "@mozilla.org/rdf/datasource;1?name=window-mediator";
    var wm = nsJSComponentManager.getService(WM_CONTRACTID, "nsIWindowMediator");
    return wm.getMostRecentWindow(aWindowType);
  },
  
  getRootShell: function ()
  {
    var main = this.getWindow("vixen:main");
    return main.vxShell;
  },
  
  positionDocumentWindow: function (aWindowType)
  {
    var prevFD = vxUtils.getWindow(aWindowType);
    var x = kPaletteWidth + 5;
    var y = kMenuHeight + 5;
    if (prevFD) {
      x = prevFD.screenX + 10;
      y = prevFD.screenY + 10;
      var w = prevFD.outerWidth;
      var h = prevFD.outerHeight;
      if ((x + w) > (screen.availWidth - kPropertiesWidth) ||
          (y + h) > (screen.availHeight)) {
        x = kPaletteWidth + 5;
        y = kMenuHeight + 5;
      }
    }
    return { x: x, y: y };
  }
};

/** 
 * dumps the DOM tree under a given node.
 */
function dumpDOM(aNode)
{
  _dd("<" + aNode.localName + " id=" + aNode.id + ">");
  for (var i = 0; i < aNode.childNodes.length; i++)
    dumpDOM(aNode.childNodes[i]);
}

/** 
 * Converts a regular js array into an nsISimpleEnumerator
 */
function ArrayEnumerator(aArray)
{
  this.mArray = aArray;
  this.mIndex = 0;
}

ArrayEnumerator.prototype = {
  getNext: function ()
  {
    return this.mArray[this.mIndex++];
  },
  
  hasMoreElements: function ()
  {
    return this.mIndex < this.mArray.length;
  }
};

/** 
 * RDF Node object
 */
function RDFNode()
{
}

RDFNode.prototype = {
  EqualsNode: function (aRDFNode) 
  {
    try {
      var node = aRDFNode.QueryInterface(Components.interfaces.nsIRDFLiteral);
      if (node.Value == this.Value) 
        return true;
    }
    catch (e) {
    }
    return false;
  }
}; 
 
/** 
 * RDF Literal object
 */
function RDFLiteral (aValue)
{
  this.Value = aValue;
}

RDFLiteral.prototype = 
{
  __proto__: RDFNode.prototype
};

/** 
 * RDF Resource object
 */
function RDFResource (aValue)
{
  this.Value = aValue;
}

RDFResource.prototype = 
{ 
  __proto__: RDFNode.prototype
};


