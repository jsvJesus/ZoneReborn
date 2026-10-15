class gfx.core.UIComponent extends MovieClip
{
   var dispatchEvent;
   var invalidationIntervalID;
   var initialized = false;
   var __width = 0;
   var __height = 0;
   var _disabled = false;
   var _focused = false;
   var _displayFocus = false;
   var sizeIsInvalid = false;
   function UIComponent()
   {
      super();
      gfx.events.EventDispatcher.initialize(this);
   }
   function onLoad()
   {
      if(this.__width == 0)
      {
         this.setSize(this._width,this._height);
      }
      this.initialized = true;
      this.configUI();
      this.validateNow();
      if(this._focused && Selection.getFocus() == null)
      {
         gfx.managers.FocusHandler.instance.onSetFocus(null,this);
      }
   }
   function get disabled()
   {
      return this._disabled;
   }
   function set disabled(value)
   {
      this._disabled = value;
      super.enabled = !value;
      this.useHandCursor = !value;
      this.invalidate();
   }
   function get visible()
   {
      return this._visible;
   }
   function set visible(value)
   {
      if(this._visible == value)
      {
         return;
      }
      this._visible = value;
      if(!this.initialized)
      {
         return;
      }
      var eventType = !value ? "hide" : "show";
      this.dispatchEvent({type:eventType});
   }
   function get width()
   {
      return this.__width;
   }
   function set width(value)
   {
      this.setSize(value,this.__height || this._height);
   }
   function get height()
   {
      return this.__height;
   }
   function set height(value)
   {
      this.setSize(this.__width || this._width,value);
   }
   function setSize(width, height)
   {
      if(this.__width == width && this.__height == height)
      {
         return undefined;
      }
      this.__width = width;
      this.__height = height;
      this.sizeIsInvalid = true;
      this.invalidate();
   }
   function get focused()
   {
      return this._focused;
   }
   function set focused(value)
   {
      if(value == this._focused)
      {
         return;
      }
      this._focused = value;
      if(!value && Selection.getFocus() == this)
      {
         Selection.setFocus(null);
      }
      else if(value && Selection.getFocus() != this)
      {
         Selection.setFocus(this);
      }
      this.changeFocus();
      var eventType = !value ? "focusOut" : "focusIn";
      this.dispatchEvent({type:eventType});
   }
   function get displayFocus()
   {
      return this._displayFocus;
   }
   function set displayFocus(value)
   {
      if(value == this._displayFocus)
      {
         return;
      }
      this._displayFocus = value;
      this.changeFocus();
   }
   function handleInput(details, pathToFocus)
   {
      if(pathToFocus != null && pathToFocus.length > 0)
      {
         var handled = pathToFocus[0].handleInput(details,pathToFocus.slice(1));
         if(handled)
         {
            return handled;
         }
      }
      return false;
   }
   function invalidate()
   {
      if(this.invalidationIntervalID)
      {
         return undefined;
      }
      this.invalidationIntervalID = setInterval(this,"validateNow",1);
   }
   function validateNow()
   {
      if(this.invalidationIntervalID == null)
      {
         return undefined;
      }
      clearInterval(this.invalidationIntervalID);
      delete this.invalidationIntervalID;
      this.draw();
      this.sizeIsInvalid = false;
   }
   function toString()
   {
      return "[Scaleform UIComponent " + this._name + "]";
   }
   function configUI()
   {
   }
   function initSize()
   {
      var w = this.__width != 0 ? this.__width : this._width;
      var h = this.__height != 0 ? this.__height : this._height;
      this._xscale = this._yscale = 100;
      this.setSize(w,h);
   }
   function draw()
   {
   }
   function changeFocus()
   {
   }
   function onMouseWheel(delta, target)
   {
      if(this.visible && this.hitTest(_root._xmouse,_root._ymouse,true))
      {
         var obj = Mouse.getTopMostEntity();
         while(obj)
         {
            if(obj == this)
            {
               this.scrollWheel(delta <= 0 ? -1 : 1);
               break;
            }
            obj = obj._parent;
         }
      }
   }
   function scrollWheel(delta)
   {
   }
}
