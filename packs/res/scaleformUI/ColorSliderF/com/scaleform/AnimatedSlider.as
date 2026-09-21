class com.scaleform.AnimatedSlider extends gfx.controls.Slider
{
   var thumb;
   var dispatchEvent;
   var dragOffset;
   var track;
   var speed = 0.5;
   var _islive = true;
   function AnimatedSlider()
   {
      super();
      gfx.motion.Tween.init();
   }
   function updateThumb(newValue)
   {
      if(newValue == null)
      {
         newValue = this._value;
      }
      if(this._islive)
      {
         var _loc2_ = (this._value - this._minimum) / (this._maximum - this._minimum) * this.__width - this.thumb._width / 2;
         MovieClip(this.thumb).tweenTo(this.speed,{_x:_loc2_},mx.transitions.easing.Strong.easeInOut);
         this.dispatchEvent({type:"change"});
      }
      else
      {
         this.thumb._x = (this._value - this._minimum) / (this._maximum - this._minimum) * this.__width - this.thumb._width / 2;
      }
   }
   function beginDrag(event)
   {
      super.beginDrag();
      this._islive = false;
   }
   function doDrag()
   {
      var _loc3_ = this._xmouse - this.dragOffset.x;
      var _loc2_ = this.lockValue(_loc3_ / this.__width * (this._maximum - this._minimum) + this._minimum);
      this.updateThumb(_loc2_);
      if(this.value == _loc2_)
      {
         return undefined;
      }
      this._value = _loc2_;
      if(this.liveDragging)
      {
         this.dispatchEvent({type:"change"});
      }
   }
   function endDrag()
   {
      super.endDrag();
      this._islive = true;
   }
   function trackPress(e)
   {
      Selection.setFocus(this.track);
      var _loc2_ = this.lockValue(this._xmouse / this.__width * (this._maximum - this._minimum) + this._minimum);
      if(this.value == _loc2_)
      {
         return undefined;
      }
      this.value = _loc2_;
   }
}
