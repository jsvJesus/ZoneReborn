class gfx.controls.Label extends gfx.core.UIComponent
{
   var _text;
   var isHtml;
   var textField;
   var constraints;
   var dispatchEvent;
   var _autoSize = false;
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
      if(this.textField != null)
      {
         this.textField.text = value;
      }
      if(this._autoSize && this.initialized)
      {
         this.__width = this._width = this.calculateWidth();
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
      if(this._autoSize && this.initialized)
      {
         this.__width = this._width = this.calculateWidth();
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
      if(this._autoSize && this.initialized)
      {
         this.width = this.calculateWidth();
      }
   }
   function setSize(width, height)
   {
      var w = !this._autoSize ? width : this.calculateWidth();
      super.setSize(w,height);
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
   }
   function calculateWidth()
   {
      var metrics = this.constraints.getElement(this.textField).metrics;
      return this.textField.textWidth + metrics.left + metrics.right + 5;
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
