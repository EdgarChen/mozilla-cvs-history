
//
// return a string representing the html content in html format 
//
function htmlString(node, indent)
{
    var html = ""
    indent += "  "

    var type = node.GetNodeType()
    if (type == Node.ELEMENT) {

        // open tag
        html += indent + "<" + node.GetTagName()

        // dump the attributes if any
        attributes = node.GetAttributes()
        if (null != attributes) {
            html += " "
            var countAttrs = attributes.GetLength()
            var index = 0
            while(index < countAttrs) {
                att = attributes.Item(index)
                if (null != att) {
                    html += att.ToString()
                }
                index++
            }
        }

        // end tag
        html += ">"

        // recursively dump the children
        if (node.HasChildNodes()) {
            // get the children
            var children = node.GetChildNodes()
            var length = children.GetLength()
            var child = children.GetNextNode()
            var count = 0;
            while(count < length) {
                html += htmlString(child, indent)
                child = children.GetNextNode()
                count++
            }
        }

        // close tag
        html += "</" + node.GetTagName() + ">"
    }
    // if it's a piece of text just dump the text
    else if (type == Node.TEXT) {
        html += node.data
    }

    return html;
}

htmlString(document.documentElement, "") 


 