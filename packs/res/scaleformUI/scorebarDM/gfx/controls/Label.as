class gfx.controls.Label extends gfx.core.UIComponent
{
   var _text;
   var isHtml;
   var textField;
   var constraints;
   var dispatchEvent;
   var _autoSize = "none";
   function Label()
   {
      super();
   }
   function get textID()
   {
      return null;
   }
   function set textID(value)
   {
      if(value != "")
      {
         this.text = gfx.utils.Locale.getTranslatedString(value);
      }
   }
   function get text()
   {
      return this._text;
   }
   function set text(value)
   {
      this.isHtml = false;
      this._text = value;
      if(this.initialized)
      {
         if(this.textField != null)
         {
            this.textField.text = value;
         }
         this.sizeIsInvalid = true;
         this.validateNow();
      }
   }
   function get htmlText()
   {
      return this._text;
   }
   function set htmlText(value)
   {
      this.isHtml = true;
      this._text = value;
      if(this.textField != null)
      {
         this.textField.html = true;
         this.textField.htmlText = value;
      }
      if(this.initialized)
      {
         this.sizeIsInvalid = true;
         this.validateNow();
      }
   }
   function get disabled()
   {
      return this._disabled;
   }
   function set disabled(value)
   {
      super.disabled = value;
      this.setState();
   }
   function get autoSize()
   {
      return this._autoSize;
   }
   function set autoSize(value)
   {
      if(this._autoSize == value)
      {
         return;
      }
      this._autoSize = value;
      if(this.initialized)
      {
         this.sizeIsInvalid = true;
         this.validateNow();
      }
   }
   function setSize(width, height)
   {
      var _loc3_ = this._autoSize == "none" ? width : this.calculateWidth();
      super.setSize(_loc3_,height);
   }
   function toString()
   {
      return "[Scaleform Label " + this._name + "]";
   }
   function configUI()
   {
      this.constraints = new gfx.utils.Constraints(this,true);
      this.constraints.addElement(this.textField,gfx.utils.Constraints.ALL);
      this.tabEnabled = this.tabChildren = false;
      super.configUI();
      this.updateAfterStateChange();
      if(this.autoSize != "none")
      {
         this.sizeIsInvalid = true;
      }
   }
   function alignForAutoSize()
   {
      if(!this.initialized || this._autoSize == "none" || this.textField == null)
      {
         return undefined;
      }
      var _loc2_ = this.__width;
      this.width = this.calculateWidth();
      switch(this._autoSize)
      {
         case "right":
            var _loc3_ = this._x + _loc2_;
            this._x = _loc3_ - this.__width;
            break;
         case "center":
            var _loc4_ = this._x + _loc2_ / 2;
            this._x = _loc4_ - this.__width / 2;
      }
   }
   function calculateWidth()
   {
      if(!this.initialized)
      {
         return this.textField.textWidth + 5;
      }
      var _loc2_ = this.constraints.getElement(this.textField).metrics;
      return this.textField.textWidth + _loc2_.left + _loc2_.right + 5;
   }
   function updateAfterStateChange()
   {
      if(!this.initialized)
      {
         return undefined;
      }
      this.validateNow();
      if(this.textField != null && this._text != null)
      {
         if(this.isHtml)
         {
            this.textField.html = true;
            this.textField.htmlText = this._text;
         }
         else
         {
            this.textField.text = this._text;
         }
      }
      if(this.constraints != null)
      {
         this.constraints.update(this.__width,this.__height);
      }
      this.dispatchEvent({type:"stateChange",state:(!this._disabled ? "default" : "disabled")});
   }
   function draw()
   {
      if(this.sizeIsInvalid)
      {
         this.alignForAutoSize();
         this._width = this.__width;
         this._height = this.__height;
      }
      this.constraints.update(this.__width,this.__height);
   }
   function setState()
   {
      this.gotoAndPlay(!this._disabled ? "default" : "disabled");
      this.updateAfterStateChange();
   }
}
