class gfx.controls.ScrollBar extends gfx.controls.ScrollIndicator
{
   var upArrow;
   var downArrow;
   var track;
   var thumb;
   var onRelease;
   var constraints;
   var pageSize;
   var onMouseMove;
   var onMouseUp;
   var dragOffset;
   var trackDragMouseIndex;
   var _scrollTarget;
   var trackScrollPageSize = 1;
   var _trackMode = "scrollPage";
   var trackScrollPosition = -1;
   function ScrollBar()
   {
      super();
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
      this.gotoAndPlay(!this._disabled ? "default" : "disabled");
      if(this.initialized)
      {
         this.upArrow.disabled = this._disabled;
         this.downArrow.disabled = this._disabled;
         this.track.disabled = this._disabled;
      }
   }
   function get position()
   {
      return super.position;
   }
   function set position(value)
   {
      value = Math.round(value);
      if(value == this.position)
      {
         return;
      }
      super.position = value;
      this.updateScrollTarget();
   }
   function get trackMode()
   {
      return this._trackMode;
   }
   function set trackMode(value)
   {
      if(value == this._trackMode)
      {
         return;
      }
      this._trackMode = value;
      if(this.initialized)
      {
         this.track.autoRepeat = this.trackMode == "scrollPage";
      }
   }
   function get availableHeight()
   {
      return this.track.height - this.thumb.height + this.offsetBottom + this.offsetTop;
   }
   function toString()
   {
      return "[Scaleform ScrollBar " + this._name + "]";
   }
   function configUI()
   {
      super.configUI();
      delete this.onRelease;
      if(this.downArrow == null)
      {
         this.downArrow = new gfx.controls.Button();
      }
      if(this.upArrow == null)
      {
         this.upArrow = new gfx.controls.Button();
      }
      this.upArrow.addEventListener("click",this,"scrollUp");
      this.downArrow.addEventListener("click",this,"scrollDown");
      this.thumb.addEventListener("press",this,"beginDrag");
      this.track.addEventListener("press",this,"beginTrackScroll");
      this.track.addEventListener("click",this,"trackScroll");
      this.upArrow.useHandCursor = !this._disabled;
      this.downArrow.useHandCursor = !this._disabled;
      this.thumb.useHandCursor = !this._disabled;
      this.upArrow.disabled = this._disabled;
      this.downArrow.disabled = this._disabled;
      this.track.disabled = this._disabled;
      this.upArrow.focusTarget = this;
      this.downArrow.focusTarget = this;
      this.upArrow.autoRepeat = true;
      this.downArrow.autoRepeat = true;
      this.track.autoRepeat = this.trackMode == "scrollPage";
      this.thumb.lockDragStateChange = true;
      Mouse.addListener(this);
      var r = this._rotation;
      this._rotation = 0;
      this.constraints = new gfx.utils.Constraints(this);
      this.constraints.addElement(this.downArrow,gfx.utils.Constraints.BOTTOM);
      this.constraints.addElement(this.track,gfx.utils.Constraints.TOP | gfx.utils.Constraints.BOTTOM);
      this._rotation = r;
   }
   function draw()
   {
      if(this.direction == "horizontal")
      {
         this.constraints.update(this.__height,this.__width);
      }
      else
      {
         this.constraints.update(this.__width,this.__height);
      }
      this.updateThumb();
   }
   function updateThumb()
   {
      if(!this.initialized)
      {
         this.invalidate();
         return undefined;
      }
      if(this._disabled)
      {
         return undefined;
      }
      var per = Math.max(1,this.maxPosition - this.minPosition + this.pageSize);
      var trackHeight = this.track.height + this.offsetTop + this.offsetBottom;
      var space = trackHeight;
      this.thumb.height = Math.max(10,Math.min(trackHeight,this.pageSize / per * space));
      var percent = (this._position - this.minPosition) / (this.maxPosition - this.minPosition);
      var top = this.track._y - this.offsetTop;
      var yPos = percent * this.availableHeight + top;
      this.thumb._y = Math.max(top,Math.min(this.track._y + this.track.height - this.thumb.height + this.offsetBottom,yPos));
      this.thumb.visible = !(isNaN(percent) || this.maxPosition <= 0 || this.maxPosition == Infinity);
      if(this.thumb.visible)
      {
         this.track.disabled = false;
         if(this._position == this.minPosition)
         {
            this.upArrow.disabled = true;
         }
         else
         {
            this.upArrow.disabled = false;
         }
         if(this._position == this.maxPosition)
         {
            this.downArrow.disabled = true;
         }
         else
         {
            this.downArrow.disabled = false;
         }
      }
      else
      {
         this.upArrow.disabled = true;
         this.downArrow.disabled = true;
         this.track.disabled = true;
      }
   }
   function scrollUp()
   {
      this.position = this.position - 1;
   }
   function scrollDown()
   {
      this.position = this.position + 1;
   }
   function beginDrag()
   {
      if(this.isDragging == true)
      {
         return undefined;
      }
      this.isDragging = true;
      this.onMouseMove = this.doDrag;
      this.onMouseUp = this.endDrag;
      this.dragOffset = {y:this._ymouse - this.thumb._y};
   }
   function doDrag()
   {
      var percent = (this._ymouse - this.dragOffset.y - this.track._y) / this.availableHeight;
      this.position = this.minPosition + percent * (this.maxPosition - this.minPosition);
   }
   function endDrag()
   {
      delete this.onMouseUp;
      delete this.onMouseMove;
      this.isDragging = false;
      if(this.trackDragMouseIndex != undefined)
      {
         if(!this.thumb.hitTest(_root._xmouse,_root._ymouse))
         {
            this.thumb.onReleaseOutside(this.trackDragMouseIndex);
         }
         else
         {
            this.thumb.onRelease(this.trackDragMouseIndex);
         }
      }
      delete this.trackDragMouseIndex;
   }
   function beginTrackScroll(e)
   {
      var percent = (this._ymouse - this.thumb.height / 2 - this.track._y) / this.availableHeight;
      this.trackScrollPosition = Math.round(percent * (this.maxPosition - this.minPosition) + this.minPosition);
      if(Key.isDown(16) || this.trackMode == "scrollToCursor")
      {
         this.position = this.trackScrollPosition;
         this.trackDragMouseIndex = e.mouseIndex;
         this.thumb.onPress(this.trackDragMouseIndex);
         this.dragOffset = {y:this.thumb.height / 2};
      }
   }
   function trackScroll()
   {
      if(this.isDragging || this.position == this.trackScrollPosition)
      {
         return undefined;
      }
      this.position = this.position + (this.position >= this.trackScrollPosition ? - this.trackScrollPageSize : this.trackScrollPageSize);
   }
   function updateScrollTarget()
   {
      if(this._scrollTarget == null)
      {
         return undefined;
      }
      if(this._scrollTarget && !this._disabled)
      {
         this._scrollTarget.scroll = this._position;
      }
   }
   function scrollWheel(delta)
   {
      this.position -= delta;
   }
}
