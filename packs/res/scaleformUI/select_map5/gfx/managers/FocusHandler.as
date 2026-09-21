class gfx.managers.FocusHandler
{
   var inputDelegate;
   var currentFocus;
   var actualFocus;
   static var _instance = gfx.managers.FocusHandler.instance;
   var inited = false;
   function FocusHandler()
   {
      Selection.addListener(this);
      _global.gfxExtensions = 1;
      Selection.alwaysEnableArrowKeys = true;
      Selection.disableFocusKeys = true;
      Selection.disableFocusAutoRelease = true;
      Selection.disableFocusRolloverEvent = true;
      _root._focusrect = false;
   }
   static function get instance()
   {
      if(gfx.managers.FocusHandler._instance == null)
      {
         gfx.managers.FocusHandler._instance = new gfx.managers.FocusHandler();
      }
      return gfx.managers.FocusHandler._instance;
   }
   function initialize()
   {
      this.inited = true;
      this.inputDelegate = gfx.managers.InputDelegate.instance;
      this.inputDelegate.addEventListener("input",this,"handleInput");
   }
   function getFocus()
   {
      return this.currentFocus;
   }
   function setFocus(focus)
   {
      if(!this.inited)
      {
         this.initialize();
      }
      while(focus.focusTarget != null)
      {
         focus = focus.focusTarget;
      }
      if(this.currentFocus != focus)
      {
         this.currentFocus.focused = false;
         this.currentFocus = focus;
         this.currentFocus.focused = true;
      }
      if(this.actualFocus != this.currentFocus && !(this.actualFocus instanceof TextField))
      {
         Selection.setFocus(this.currentFocus);
      }
   }
   function handleInput(event)
   {
      var path = this.getPathToFocus();
      if(path.length == 0 || path[0].handleInput == null || path[0].handleInput(event.details,path.slice(1)) != true)
      {
         if(event.details.value == "keyUp")
         {
            return undefined;
         }
         var nav = event.details.navEquivalent;
         if(nav != null)
         {
            if(this.actualFocus instanceof TextField && eval(Selection.getFocus()) == this.actualFocus && this.textFieldHandleInput(nav))
            {
               return undefined;
            }
            var newFocus = MovieClip(Selection.moveFocus(nav,this.currentFocus));
         }
      }
   }
   function getPathToFocus()
   {
      var f = this.currentFocus;
      var path = [f];
      while(f)
      {
         f = f._parent;
         if(f.handleInput != null)
         {
            path.unshift(f);
         }
         if(f == _root)
         {
            break;
         }
      }
      return path;
   }
   function onSetFocus(oldFocus, newFocus)
   {
      if(oldFocus instanceof TextField && newFocus == null)
      {
         return undefined;
      }
      this.actualFocus = newFocus;
      this.setFocus(newFocus);
   }
   function textFieldHandleInput(nav)
   {
      var position = Selection.getCaretIndex();
      switch(nav)
      {
         case gfx.ui.NavigationCode.UP:
            if(!this.actualFocus.multiline)
            {
               return false;
            }
            break;
         case gfx.ui.NavigationCode.LEFT:
            break;
         case gfx.ui.NavigationCode.DOWN:
            if(!this.actualFocus.multiline)
            {
               return false;
            }
         case gfx.ui.NavigationCode.RIGHT:
            return position < TextField(this.actualFocus).length;
         default:
            return false;
      }
      return position > 0;
   }
}
