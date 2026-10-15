class gfx.controls.NumericStepper extends gfx.core.UIComponent
{
   var dispatchEvent;
   var nextBtn;
   var prevBtn;
   var _labelFunction;
   var constraints;
   var textField;
   var stepSize = 1;
   var _maximum = 10;
   var _minimum = 0;
   var _value = 0;
   function NumericStepper()
   {
      super();
      this.tabChildren = false;
      this.focusEnabled = this.tabEnabled = !this._disabled;
   }
   function get maximum()
   {
      return this._maximum;
   }
   function set maximum(value)
   {
      this._maximum = value;
      value = this._value;
   }
   function get minimum()
   {
      return this._minimum;
   }
   function set minimum(value)
   {
      this._minimum = value;
      value = this._value;
   }
   function get value()
   {
      return this._value;
   }
   function set value(v)
   {
      v = this.lockValue(v);
      if(v == this._value)
      {
         return;
      }
      this._value = v;
      if(this.initialized)
      {
         this.dispatchEvent({type:"change"});
      }
      this.invalidate();
   }
   function get disabled()
   {
      return this._disabled;
   }
   function set disabled(value)
   {
      if(this._disabled == value)
      {
         return;
      }
      super.disabled = value;
      this.focusEnabled = this.tabEnabled = !this._disabled;
      this.gotoAndPlay(!this._disabled ? (!this._focused ? "default" : "focused") : "disabled");
      if(!this.initialized)
      {
         return;
      }
      this.updateAfterStateChange();
      this.prevBtn.disabled = this.nextBtn.disabled = this._disabled;
   }
   function get labelFunction()
   {
      return this._labelFunction;
   }
   function set labelFunction(value)
   {
      this._labelFunction = value;
      this.updateLabel();
   }
   function increment()
   {
      this.onNext(null);
   }
   function decrement()
   {
      this.onPrev(null);
   }
   function handleInput(details, pathToFocus)
   {
      var keyPress = details.value == "keyDown";
      switch(details.navEquivalent)
      {
         case gfx.ui.NavigationCode.RIGHT:
            if(this._value < this._maximum)
            {
               if(keyPress)
               {
                  this.onNext(null);
               }
               return true;
            }
            break;
         case gfx.ui.NavigationCode.LEFT:
            if(this._value > this._minimum)
            {
               if(keyPress)
               {
                  this.onPrev(null);
               }
               return true;
            }
            break;
         default:
            break;
         case gfx.ui.NavigationCode.HOME:
            if(!keyPress)
            {
               this.value = this._minimum;
            }
            return true;
         case gfx.ui.NavigationCode.END:
            if(!keyPress)
            {
               this.value = this._maximum;
            }
            return true;
      }
      return false;
   }
   function toString()
   {
      return "[ Scaleform NumericStepper: " + this._name + "]";
   }
   function configUI()
   {
      this.nextBtn.addEventListener("click",this,"onNext");
      this.prevBtn.addEventListener("click",this,"onPrev");
      this.nextBtn.focusTarget = this.prevBtn.focusTarget = this;
      this.nextBtn.tabEnabled = this.prevBtn.tabEnabled = false;
      this.nextBtn.autoRepeat = this.prevBtn.autoRepeat = true;
      this.prevBtn.disabled = this.nextBtn.disabled = this._disabled;
      this.constraints = new gfx.utils.Constraints(this,true);
      this.constraints.addElement(this.textField,gfx.utils.Constraints.LEFT | gfx.utils.Constraints.RIGHT);
      super.configUI();
   }
   function draw()
   {
      if(this.sizeIsInvalid)
      {
         this._width = this.__width;
         this._height = this.__height;
      }
      if(this.constraints != null)
      {
         this.constraints.update(this.__width,this.__height);
      }
      this.updateLabel();
   }
   function changeFocus()
   {
      this.gotoAndPlay(!this._disabled ? (!this._focused ? "default" : "focused") : "disabled");
      this.updateAfterStateChange();
      this.prevBtn.displayFocus = this.nextBtn.displayFocus = this._focused;
   }
   function updateAfterStateChange()
   {
      if(this.constraints != null)
      {
         this.constraints.update(this.__width,this.__height);
      }
      this.updateLabel();
      this.dispatchEvent({type:"stateChange",state:(!this._disabled ? (!this._focused ? "default" : "focused") : "disabled")});
   }
   function onNext(evtObj)
   {
      this.value = this._value + this.stepSize;
   }
   function onPrev(evtObj)
   {
      this.value = this._value - this.stepSize;
   }
   function lockValue(value)
   {
      var newVal = Math.max(this._minimum,Math.min(this._maximum,this.stepSize * Math.round(value / this.stepSize)));
      return newVal;
   }
   function updateLabel()
   {
      var label = this._value.toString();
      if(this._labelFunction != null)
      {
         label = this._labelFunction(this._value);
      }
      this.textField.text = label;
   }
}
