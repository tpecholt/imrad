![img](https://github.com/tpecholt/imrad/actions/workflows/windows.yml/badge.svg)
![img](https://github.com/tpecholt/imrad/actions/workflows/ubuntu.yml/badge.svg)
![img](https://github.com/tpecholt/imrad/actions/workflows/macos.yml/badge.svg)

# ImRAD
<div align="center">
<img src="./src/icon.png" alt="logo" width="150" align="center">
</div>
<br>

ImRAD is a GUI builder for the ImGui library. It generates and parses C++ code which can be directly used in your application.  

Unlike other tools ImRAD can be used for continuous modification of the generated UI. Data binding, events and even manual
UI code additions are well supported. 

ImRAD runs on Windows, Linux and MacOS. Generated code runs on Windows, Linux, MacOS and Android.

<hr>

![image](https://github.com/user-attachments/assets/e5896d57-0560-476b-a1cf-2d7b844c1b68)


*Take a note of the Toolbar section:*
* *Configurable window style which is stored in an INI file. It contains definitions of colors, style variables and fonts. The example uses stock "Dark" style*
* *Dialog units option which can also be set to DPI aware pixels useful when designing android apps*
* *Code Preview for quick checks. Full generated code is saved in .h/cpp files*   
* *Class wizard allows to manage member variables of the generated class*
* *Horizontal layout helper using invisible table. This is an alternative to the box layout functionality.*

*Designed window shows:*
* *Stretchable spacer between "Available fields" text and "show all" checkbox and between dialog buttons. ImRAD comes with box layout support not available in standard ImGui*
* *Negative size_x/y support which makes the table widget expand vertically keeping 48px gap from the bottom edge of the dialog*
* *Selectable widget bindings such as `{vars[i].first}`. Bind expressions will be put in the generated code showing content of the `vars` array*

*Properties window contains:*
* *Table.rowCount binding set to the `vars.size()` expression. This will generate code with a for loop over table rows. One-way or two-way binding is supported by most properties*

*Events window*
* *Allows to generate handlers for all kinds of events such as button click, window appearance, input text character filter etc.* 

# Main Features

* Supports designing all kinds of windows
  * Floating windows, popups and modal popups. These are ImGui backend independent
  * MainWindow with GLFW integration. ImRAD generates GLFW calls which will synchronize ImGui window with its OS window (title bar, resizability flags, autosize etc.)
  * Activity. This is an undecorated window which fills the entire viewport area. Only one activity can be shown at the time. Used mainly for Android apps  
  * contains a GLFW template for generating generic `main.cpp`
  * contains an android template for generating generic `MainActivity.java`, `AndroidManifest.xml`, `main.cpp` and `CMakeLists.txt`
  
* Supports wide range of widgets
  
  * basic widgets like `Text`, `Checkbox`, `Combo`, `Button`, `Slider`, `ColorEdit` etc.
  * container widgets like `Child`, `Table`, `CollapsingHeader`, `TreeNode`, `TabBar`,
  * more exotic widgets such as `Splitter` and `DockSpace`
  * `MenuBar` and context menu editing
  * `CustomWidget` which allows to call external widgets code
    
* Generates layout using `SameLine`/`Spacing`/`NextColumn` instead of absolute positioning 
  
  * this ensures widgets respect item spacing and frame padding in a consistent way
  * there is a clear relationship between parent - child widget as well as children ordering which is important for container widgets like Table
  * alternative positioning where widget keeps relative distance from a selected parent's corner is available and can be used for overlay widgets

* Supports box layout

  * powerful and simple to use layout mechanism implemented on top of ImGui functionality
  * stretch any sizeable widget in horizontal or vertical direction
  * insert spacers to achieve alignment
  * alternatively you can use Table Layout Helper to generate horizontal layout using invisible Table

* Supports property binding 
  
  * because ImGui is an immediate mode GUI library widget state like input text or combobox items must be set at the time of drawing from within the generated code through property binding. 
  * class variables can be managed through simple class wizard or from binding dialog
  * using property binding generated UI becomes dynamic and yet it can still be designed at the same time  

* Supports generating event handlers and other support code
  
  * for example modal dialog will automatically generate an `OpenPopup` member function with a lambda callback called when the dialog is closed
  * event handlers allow event handling user code to be separated from the generated part so the designer still works

* Generated code is delimited by comment markers and user can insert additional code around and continue to use ImRAD at the same time
  
  * this can be used to f.e. to call draw dependent popups or to calculate some variables
  * it is also possible to use `CustomWidget` which will delegate to a user callback 

* Target window style is fully configurable
  * apart from default styles provided by ImGui user can define new style and save it as an INI file under the `style` folder. Colors, style variables and fonts can all be configured.
  * ImRAD will follow chosen style settings when designing your UI
  * stored style can be loaded in your app by calling `ImRad::LoadStyle`  

* Generated code is ready to use in your project and depends only on ImGui library and one accompanying header file **imrad.h**
  * Current **imrad.h** contains both the interface and implementation. Define `IMRAD_H_IMPLEMENTATION` prior including it in the main.cpp file to create implementation.
  * Generated code uses `ImRad::Format` formatting routines. In C++20 this will delegate to `std::format` by default but when requested popular `fmt` library can be used instead by defining `IMRAD_WITH_FMT`. If neither is available simple formatting routine which skips formatting flags will be used.
  * Some features such as `MainWindow` or `Image` widget require additional library dependencies (GLFW, STB) and need to be explicitly enabled by defining `IMRAD_WITH_LOAD_TEXTURE`
  * **imrad.h** supports loading assets from zipped file using the *zlib* library. Activate it by defining `IMRAD_WITH_MINIZIP`

* ImRAD tracks changes to the opened files so files can be designed in ImRAD and edited in your IDE of choice at the same time

# License

* ImRAD source code is licensed under the GPL license 
* Any code generated by the tool is excluded from GPL and can be included in any project either open-source or commercial and it's up to the user to decide the license for it. 
* Additionally since `imrad.h` is used by the generated code it is also excluded from the GPL license  

# Download binaries

For up-to date version clone & build the repository using CMake. Don't forget to fetch submodules in the 3rdparty directory too.

Somewhat older version can be downloaded from [Releases](https://github.com/tpecholt/imrad/releases)

# How to build

## Windows
1. Use CMake GUI to configure and generate sln file
2. Open the generated sln file in Visual Studio 2017 or newer (you can use Express or Community editions which are downloadable for free)
3. Build the INSTALL project in Release mode. It may require running VS with admin rights
4. If you didn't alter CMAKE_INSTALL_PREFIX variable ImRAD will be installed into *C:\Program Files\imrad\latest*

## Linux
1. *Ubuntu*: Due to the GTK FileOpen dialog dependency you need to `apt install` these packages first (exact list depends on your OS):
   
   *pkg-config libgtk-3-dev libsystemd-dev libwebp-dev libzstd-dev libssl-dev*

3. Run the provided installation script (script parameter is the ImRAD version you want to name the folder) 

   ```./release-linux 0.9```

4. ImRAD will be installed into *./install/imrad-0.9*

## MacOS

1. Build using CMake. Please look at the macos action for an inspiration.

# How to debug
   
## Windows

1. Build the INSTALL target in VS as described above
2. Set imrad as startup project, set its working directory to the installed folder
3. Debug & Run

# Tutorials & How to

Please check [wiki](https://github.com/tpecholt/imrad/wiki) for tutorials and more detailed content. There is a lot to discover!

# Sponsorship

Development of ImRAD happens in my free time outside of my regular job, yoga, badminton, volunteering and other activities. 

If you like the tool and want to support its further development please do. If you use it regularly you can even setup a monthly donation if it's convenient for you.

[![kofi_button_black](https://github.com/user-attachments/assets/67a4a74a-0c06-425a-be98-55da20f05181)](https://ko-fi.com/tope99)

# Credits

Design and implementation - [Tomas Pecholt](https://github.com/tpecholt)

Thanks to [Omar Cornut for Dear ImGui](https://github.com/ocornut/imgui)
