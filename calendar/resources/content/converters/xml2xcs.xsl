<?xml version="1.0"?>
<!-- ***** BEGIN LICENSE BLOCK *****
   - Version: MPL 1.1/GPL 2.0/LGPL 2.1
   -
   - The contents of this file are subject to the Mozilla Public License Version
   - 1.1 (the "License"); you may not use this file except in compliance with
   - the License. You may obtain a copy of the License at
   - http://www.mozilla.org/MPL/
   -
   - Software distributed under the License is distributed on an "AS IS" basis,
   - WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
   - for the specific language governing rights and limitations under the
   - License.
   -
   - The Original Code is xCal test tools - xml2xcs.xcs.
   -
   - The Initial Developer of the Original Code is
   - Jefferson Software Inc.
   - Portions created by the Initial Developer are Copyright (C) 2002
   - the Initial Developer. All Rights Reserved.
   -
   - Contributor(s): Gary Frederick <gary.frederick@jsoft.com>
   -
   - Alternatively, the contents of this file may be used under the terms of
   - either the GNU General Public License Version 2 or later (the "GPL"), or
   - the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
   - in which case the provisions of the GPL or the LGPL are applicable instead
   - of those above. If you wish to allow use of your version of this file only
   - under the terms of either the GPL or the LGPL, and not to allow others to
   - use your version of this file under the terms of the MPL, indicate your
   - decision by deleting the provisions above and replace them with the notice
   - and other provisions required by the LGPL or the GPL. If you do not delete
   - the provisions above, a recipient may use your version of this file under
   - the terms of any one of the MPL, the GPL or the LGPL.
   -
   - ***** END LICENSE BLOCK ***** -->
<!-- xcs2csv.xsl Version 1.0 -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:output method="xml"/>

<xsl:template match="/">
  <xsl:apply-templates select="libical" />
</xsl:template>

<xsl:template match="libical">
  <iCalendar>
    <xsl:apply-templates select="component[translate(@name,'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz')='vcalendar']" />
  </iCalendar>
</xsl:template>

<xsl:template match="component[translate(@name,'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz')='vcalendar']">
  <vcalendar>
    <xsl:apply-templates select="property" />
    <xsl:apply-templates select="component" />
  </vcalendar>
</xsl:template>

<xsl:template match="component[translate(@name,'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz')='vevent']">
  <vevent>
    <xsl:apply-templates select="property" />
  </vevent>
</xsl:template>

<xsl:template match="component[translate(@name,'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz')='vtodo']">
  <vtodo>
    <xsl:apply-templates select="property" />
  </vtodo>
</xsl:template>


<xsl:template match="component[translate(@name,'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz')='vjournal']">
  <vjournal>
    <xsl:apply-templates select="property" />
  </vjournal>
</xsl:template>

<xsl:template match="property">
    <xsl:element name="{translate(@name,'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz')}" >
      <xsl:if test="value/@value">
        <xsl:attribute name="value">
          <xsl:value-of select="value/@value" />
        </xsl:attribute>
      </xsl:if>
      <!-- ggf hack -->
      <xsl:if test="@name='CATEGORIES'">
        <item>
          <xsl:value-of select="value" />
        </item>
      </xsl:if>
      <xsl:if test="not (@name='CATEGORIES')">
        <xsl:value-of select="value" />
      </xsl:if>
    </xsl:element>
</xsl:template>

</xsl:stylesheet>
