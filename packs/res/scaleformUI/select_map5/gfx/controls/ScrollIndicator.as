class gfx.controls.ScrollIndicator extends gfx.core.UIComponent
{
   var thumb;
   var pageSize;
   var pageScrollSize;
   var dispatchEvent;
   var _scrollTarget;
   var focusTarget;
   var track;
   var onRelease;
   var inspectableScrollTarget;
   var lastVScrollPos;
   var scrollerIntervalID;
   var direction = "vertical";
   var minPosition = 0;
   var maxPosition = 10;
   var _position = 5;
   var offsetTop = 0;
   var offsetBottom = 0;
   var isDragging = false;
   function ScrollIndicator()
   {
      super();
      this.tabChildren = false;
      this.focusEnabled = this.tabEnabled = !this._disabled;
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
      if(this.initialized)
      {
         this.thumb.disabled = this._disabled;
      }
   }
   function setScrollProperties(pageSize, minPosition, maxPosition, pageScrollSize)
   {
      this.pageSize = pageSize;
      this.pageScrollSize = pageScrollSize;
      this.minPosition = minPosition;
      this.maxPosition = maxPosition;
      this.updateThumb();
   }
   function get position()
   {
      return this._position;
   }
   function set position(value)
   {
      if(value == this._position)
      {
         return;
      }
      this._position = Math.max(this.minPosition,Math.min(this.maxPosition,value));
      this.dispatchEvent({type:"scroll",position:this._position});
      this.invalidate();
   }
   function update()
   {
   }
   function get scrollTarget()
   {
      return this._scrollTarget;
   }
   function set scrollTarget(value)
   {
      var _prevScrollTarget = this._scrollTarget;
      this._scrollTarget = value;
      if(_prevScrollTarget && value._parent != _prevScrollTarget)
      {
         _prevScrollTarget.removeListener(this);
         if(_prevScrollTarget.scrollBar != null)
         {
            _prevScrollTarget.scrollBar = null;
         }
         this.focusTarget = null;
         _prevScrollTarget.noAutoSelection = false;
      }
      if(value instanceof gfx.core.UIComponent && value.scrollBar !== null)
      {
         value.scrollBar = this;
         return;
      }
      if(this._scrollTarget == null)
      {
         return;
      }
      this._scrollTarget.addListener(this);
      this._scrollTarget.noAutoSelection = true;
      this.focusTarget = this._scrollTarget;
      this.onScroller();
   }
   function get availableHeight()
   {
      return (this.direction != "horizontal" ? this.__height : this.__width) - this.thumb.height + this.offsetBottom + this.offsetTop;
   }
   function toString()
   {
      return "[Scaleform ScrollIndicator " + this._name + "]";
   }
   function configUI()
   {
      super.configUI();
      if(this.track == null)
      {
         this.track = new gfx.controls.Button();
      }
      this.thumb.focusTarget = this;
      this.track.focusTarget = this;
      this.thumb.disabled = this._disabled;
      this.onRelease = function()
      {
      };
      this.useHandCursor = false;
      this.initSize();
      this.direction = this._rotation == 0 ? "vertical" : "horizontal";
      if(this.inspectableScrollTarget != null)
      {
         var target = this._parent[this.inspectableScrollTarget];
         if(target != null)
         {
            this.scrollTarget = target;
         }
         this.inspectableScrollTarget = null;
      }
   }
   function draw()
   {
      this.track._height = this.direction != "horizontal" ? this.__height : this.__width;
      this.updateThumb();
   }
   function updateThumb()
   {
      if(!this.thumb.initialized)
      {
         this.invalidate();
         return undefined;
      }
      if(this._disabled)
      {
         return undefined;
      }
      var per = Math.max(1,this.maxPosition - this.minPosition + this.pageSize);
      var trackHeight = (this.direction != "horizontal" ? this.__height : this.__width) + this.offsetTop + this.offsetBottom;
      this.thumb.height = Math.max(10,this.pageSize / per * trackHeight);
      var percent = (this.position - this.minPosition) / (this.maxPosition - this.minPosition);
      var top = - this.offsetTop;
      var yPos = percent * this.availableHeight + top;
      this.thumb._y = Math.max(top,Math.min(trackHeight - this.offsetTop,yPos));
      this.thumb.visible = !(isNaN(percent) || this.maxPosition == 0);
   }
   function onScroller()
   {
      if(this.isDragging)
      {
         return undefined;
      }
      if(this.lastVScrollPos == this._scrollTarget.scroll)
      {
         delete this.lastVScrollPos;
         return undefined;
      }
      this.setScrollProperties(this._scrollTarget.bottomScroll - this._scrollTarget.scroll,1,this._scrollTarget.maxscroll);
      this.position = this._scrollTarget.scroll;
      this.lastVScrollPos = this._scrollTarget.scroll;
      if(this.scrollerIntervalID == undefined)
      {
         this.scrollerIntervalID = setInterval(this,"scrollerDelayUpdate",10);
      }
   }
   function scrollerDelayUpdate()
   {
      this.onScroller();
      clearInterval(this.scrollerIntervalID);
      delete this.scrollerIntervalID;
   }
}
