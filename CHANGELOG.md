# vNext

### UI Improvements

* Styles folder is watched for changes and INI style changes are applied automatically

* Separate style and units combos reworked into single configuration combo and dialog

### New Code Features

* Owner-drawn Combo through onDrawItems event

* Configurations allow to generate a set of separate Draw functions and select one at runtime. Event handlers and field variables are shared. Useful for designing UI variants like per desktop/mobile platform, by screen rotation etc.

### Improvements by Widget Type

* Combo
  
  * adds onDrawItems event


# v0.9.1

### Fix Release

* Mismatch between generated `ImGui::Begin/End` [[#58]](https://github.com/tpecholt/imrad/issues/58)

* Fix missing include in *imrad.h* on gcc-14 [[#59]](https://github.com/tpecholt/imrad/issues/59)

* Fix UI dock layout not preserved, `Input.imeMode` import, version check crash 


# v0.9

### UI Improvements

* Reworked Property grid 
  
    * Properties are better structured into Appearance/Behavior/Bindings/Common/Layout categories
    
    * Less distractive because of automatic hiding of buttons
    
    * Visualization of modified properties
    
    * Aditional edit options such as multiline text button
  
* New events can be quickly generated with double click. Default event name is suggested.
  
* Widget drag & drop. Allows to quickly reorder child widgets which was previously possible only with clipboard operations
  
* Alt+arrows move widget around in the hieararchy. This is a requested wxFormBuilder feature

* KeypadDecimal always inputs '.'. This means binding expressions with decimal numbers typed on the keypad will be correct under any OS keyboard settings
  
* Added context menu to File Tabs, make them reorderable
  
* Widget insertion became more accurate & flexible
  
* Visualization of format string arguments
  
* File Explorer added
  
* Table Columns dialog reworked. Columns can now specify flags and visibility
  
* Label-like properties allow multi-line edit through a dialog

* Error dialog shows loading/saving errors in a structured way. Message Boxes now come with an icon.
  
* Added Edit style button in the toolbar

* System monitor scaling is now used to scale fonts automatically

* Added version check (requires openssl on ubuntu)

* Add New File dialog


### New Code features

* `DockSpace`, `DockNode` widgets added
  
* DragSource/Target events added
  
* `ImRad::Format` delegates to `std::format` when available
  
* Widget shortcuts now support `RouteGlobal`, `Repeat` flags
  
* Many two way binding properties now allow to bind into arrays (`std::vector`/`array`/`span`) and specify binding expression using the Binding dialog
  
* Bindings into array elements can now use $item and $index expressions so the array and index variable names don't need to be used directly
  
* Certain bindings now support both l-values and r-values and the appropriate code will be generated (such as for `Selectable`, `RadioButton` which come in two overloads)
  
* Android template implements haptic feedback for long press events
  
* overlayPos mode is now explicit and adds anchoring to horizontal/vertical center
  
* New fields can set member visibility - interface / implementation

* BREAKING: Support of ImGui dynamic fonts - font size is now specified separately. Zoom feature fixed. Generated code requires ImGui 1.92 and up.

* initialFocus / forceFocus moved into Common section so it's available to all widgets

* BREAKING: `ImRad::IOUserData` was moved inside `ImRad::GetUserData()` so it's no longer required to instantiate it and pass it to `ImGui::IO::UserData`. Use `ImRad::GetUserData()` directly.

* Added multiSelection support for `Table`, `Child`, `Selectable`

* Added experimental support for loading assets from zip files. Activate it by defining `IMRAD_WITH_MINIZIP` which requires
the zlib library to be found. After that `LoadTextureFromFile` and font loading code in `LoadStyle` will accept file names with zip: scheme.

* Added DragDropSourceLongPressed event

* BREAKING: `imrad.h` was refactored into interface and implementation parts. By default only the interface part is included so
`imgui_internal.h` and some other headers used only for the implementation are not leaked and generated code possibly compiles faster. In one of the cpp files #define `IMRAD_H_IMPLEMENTATION` before including `imrad.h` as the last include to create implementation.

* BREAKING: `IMRAD_WITH_GLFW` and `IMRAD_WITH_STB` are no longer used. Use `IMRAD_WITH_LOAD_TEXTURE` to implemented `LoadTextureFromFile` function which is needed for the Image widget. It will require to include stb_image.h and GL/ES headers.

* BREAKING: On android the required extern function `GetAssetData` was changed into `GetAndroidAsset` with more convenient signature.

### Improvements by Widget Type

* Child
  
  * new flags like ResizeX, ResizeY, NoNavFocus
    
  * multi-selection support added

* CustomWidget

  * label added
    
* DockSpace/Node
  
  * newly added widgets (requires imgui/docking branch)

* Image
  
  * Allows to specify image resource inside a zip file
  
* Input
  
  * allows to set member function callback like character filter, auto completion etc.
    
  * new flags such as ElideLeft, DisplayEmptyRefVal, AllowTabInput, WordWrap added
    
* Selectable
  
  * ModalResult now allows to end a modal popup with a result code like Button does
    
  * LongPressed event added
    
  * BREAKING: by default ImRad::Selectable will only use as much space as needed for its label. This fixes issues when Selectables are placed inside box sizers. User can still request expansion by typying -1 directly
  
  * multi-selection support added
  
  * Selectable now behaves as a container so it's possible to insert additional child widgets which will be automatically clipped. Useful for implementing structured table items with row selection 

* Separator
  
  * added alignment, thickness for labeled separators
    
* Slider
  
  * new flags added

* Table
  
  * ScrollFreeze property implemented
    
  * New column flags such as DefaultHide, DefaultSort/NoSortAsc/Desc, NoHeaderLabel
  
  * Column visibility implemented
  
  * Separate headerFont added
    
  * multi-selection support added

* Text
  
  * Link property added
  
* TreeNode
  
  * new flags like DefaultOpen, NoTreePushOnOpen
  
  * Window
  
  * minimumSize added
  
  * innerSpacing added
    
  * new flags like NoFocusOnAppearing
  
  
### Tutorials

  * Creating a New App added

  * Data Binding & Events added

  * tutorials updated
  
  