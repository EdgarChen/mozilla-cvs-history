// OnOK(), Undo(), and Cancel() are in EdDialogCommon.js
// applyChanges() must be implemented here

var appCore;
var toolkitCore;
var insertNew = true;
var undoCount = 0;
var imageElement;
var tagName = "img"

// dialog initialization code
function Startup()
{
  dump("Doing Startup...\n");
  toolkitCore = XPAppCoresManager.Find("ToolkitCore");
  if (!toolkitCore) {
    toolkitCore = new ToolkitCore();
    if (toolkitCore)
      toolkitCore.Init("ToolkitCore");
  }
  if(!toolkitCore) {
    dump("toolkitCore not found!!! And we can't close the dialog!\n");
  }

  // NEVER create an appcore here - we must find parent editor's
  appCore = XPAppCoresManager.Find("EditorAppCoreHTML");  
  if(!appCore || !toolkitCore) {
    dump("EditorAppCore not found!!!\n");
    toolkitCore.CloseWindow(window);
  }
  dump("EditorAppCore found for Image Properties dialog\n");

  // Create dialog object to store controls for easy access
  dialog = new Object;
  // This is the "combined" widget:
  dialog.Src = document.getElementById("image.Src");
  // Can we get at just the edit field?
  fileChild = dialog.Src.firstChild;
  if (fileChild)
  {
    dump("*** fileInput control has a child\n");
  } else {
    dump("*** fileInput control has  NO child\n");
  }

  dialog.AltText = document.getElementById("image.AltText");
  if (null == dialog.Src || 
      null == dialog.AltText )
  {
    dump("Not all dialog controls were found!!!\n");
  }
      
  initDialog();
  
  dialog.Src.focus();
  if (fileChild)
    fileChild.focus();
}

function initDialog() {
  // Get a single selected anchor element
  imageElement = appCore.getSelectedElement(tagName);

  if (imageElement) {
    // We found an element and don't need to insert one
    insertNew = false;
    dump("Found existing image\n");
  } else {
    insertNew = true;
    // We don't have an element selected, 
    //  so create one with default attributes
    dump("Element not selected - calling createElementWithDefaults\n");
    imageElement = appCore.createElementWithDefaults(tagName);
  }

  if(!imageElement)
  {
    dump("Failed to get selected element or create a new one!\n");
    //toolkitCore.CloseWindow(window);
  }
}

function applyChanges()
{
  // TODO: BE SURE Src AND AltText are completed!
  imageElement.setAttribute("src",dialog.Src.value);
  // We must convert to "file:///" format else image doesn't load!
  imageElement.setAttribute("alt",dialog.AltText.value);  
  if (insertNew) {
    dump("Src="+imageElement.getAttribute("src")+" Alt="+imageElement.getAttribute("alt")+"\n");
    appCore.insertElement(imageElement, true)
    
  }
}