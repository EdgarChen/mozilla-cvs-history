/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

// By Eric D Vaughan

package com.netscape.jsdebugging.ifcui.palomar.widget.layout;

import netscape.application.*;

public class ShapeableView extends View implements Shapeable
{
    public Size preferredSize()
    {
        LayoutManager manager = layoutManager();
        if (manager != null)
        {
            if (manager instanceof ShapeableLayoutManager)
            {
                ShapeableLayoutManager smanager = (ShapeableLayoutManager)manager;
                return smanager.preferredSize(this);
            }
        }

        return minSize();
    }

    public Size minSize()
    {
        LayoutManager manager = layoutManager();
        if (manager != null)
        {
            if (manager instanceof ShapeableLayoutManager)
            {
                ShapeableLayoutManager smanager = (ShapeableLayoutManager)manager;
                return smanager.minSize(this);
            }
        }

        return super.minSize();
    }


    public Size maxSize()
    {
        LayoutManager manager = layoutManager();
        if (manager != null)
        {
            if (manager instanceof ShapeableLayoutManager)
            {
                ShapeableLayoutManager smanager = (ShapeableLayoutManager)manager;
                return smanager.maxSize(this);
            }
        }

        return new Size(Integer.MAX_VALUE, Integer.MAX_VALUE);
    }

    /**
     * If this view has a shapeable layout manager return it
     * otherwise return null.
     */
    public ShapeableLayoutManager getShapeableLayoutManager()
    {
        LayoutManager manager = layoutManager();
        if (manager != null)
        {
           if (manager instanceof ShapeableLayoutManager)
              return (ShapeableLayoutManager)manager;
        }

        return null;
    }
}
