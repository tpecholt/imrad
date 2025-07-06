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
  
* Error dialog shows loading/saving errors in a structured way
  

### New Code features

* DockSpace, DockNode widgets added
  
* DragSource/Target events added
  
* ImRad::Format delegates to std::format when available (requires extra switch on msvc)
  
* Widget shortcuts now support RouteGlobal, Repeat flags
  
* Many two way binding properties now allow to bind into arrays (std::vector/array/span) and specify binding expression using the Binding dialog
  
* Bindings into array elements can now use $item expression
  
* Certain bindings now support both l-values and r-values and the appropriate code will be generated (such as for Selectables, RadioButtons which come in two overloads)
  
* Android template implements haptic feedback for long press events
  
* overlayPos mode is now explicit and allows anchoring to horizontal/vertical center
  
* New fields can set member visibility - interface / implementation

* BREAKING: Support of ImGui dynamic fonts - font size is now specified separately. PushFont doesn't change the current font size. Zoom feature fixed.

* initialFocus / forceFocus moved into Common section so it's available to all widgets
  

### Improvements by Widget Type

*  Window
  
  * minimumSize added
  
  * innerSpacing added
    
  * new flags like NoFocusOnAppearing
    
* Input
  
  * allows to set member function callback like character filter, auto completion etc.
    
  * new flags such as ElideLeft, DisplayEmptyRefVal, AllowTabInput added
    
* Text
  
  * Link property added
    
* Selectable
  
  * ModalResult now allows to end a modal popup with a result code like Button does
    
  * LongPressed event added
    
  * BREAKING: by default ImRad::Selectable will only use as much space as needed for its label. This fixes issues when Selectables are placed inside box sizers. User can still request expansion by typying -1 directly
  
  * Selectable now behaves as a container so it's possible to insert additional child widgets which will be automatically clipped. Useful for implementing structured table items with row selection
    
* Child
  
  * new flags like ResizeX, ResizeY, NoNavFocus
    
* Table
  
  * ScrollFreeze property implemented
    
  * New column flags such as DefaultHide, DefaultSort/NoSortAsc/Desc, NoHeaderLabel
  
  * Column visibility implemented
  
  * Separate headerFont added
    
* TreeNode
  
  * new flags like DefaultOpen, NoTreePushOnOpen
    
* Slider
  
  * new flags added
    
* DockSpace/Node
  
  * newly added widgets (requires imgui/docking branch)
  
  
### Tutorials

  * Creating a New App added

  * Data Binding & Events added

  * Widget Placement & Size updated
  
  