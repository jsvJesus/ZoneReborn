class gfx.controls.ScrollingList extends gfx.controls.CoreList
{
   var _scrollBar;
   var inspectableScrollBar;
   var container;
   var _rowHeight;
   var _dataProvider;
   var renderers;
   var wrapping = "normal";
   var autoRowCount = false;
   var _scrollPosition = 0;
   var totalRenderers = 0;
   var autoScrollBar = false;
   var margin = 1;
   var paddingTop = 0;
   var paddingBottom = 0;
   var paddingLeft = 0;
   var paddingRight = 0;
   var thumbOffsetTop = 0;
   var thumbOffsetBottom = 0;
   var thumbSizeFactor = 1;
   function ScrollingList()
   {
      super();
   }
   function get scrollBar()
   {
      return this._scrollBar;
   }
   function set scrollBar(value)
   {
      if(!this.initialized)
      {
         this.inspectableScrollBar = value;
         return;
      }
      if(this._scrollBar != null)
      {
         this._scrollBar.removeEventListener("scroll",this,"handleScroll");
         this._scrollBar.removeEventListener("change",this,"handleScroll");
         this._scrollBar.focusTarget = null;
         if(this.autoScrollBar)
         {
            this._scrollBar.removeMovieClip();
         }
      }
      this.autoScrollBar = false;
      if(typeof value == "string")
      {
         this._scrollBar = MovieClip(this._parent[value.toString()]);
         if(this._scrollBar == null)
         {
            this._scrollBar = this.container.attachMovie(value.toString(),"_scrollBar",1000,{offsetTop:this.thumbOffsetTop,offsetBottom:this.thumbOffsetBottom});
            if(this._scrollBar != null)
            {
               this.autoScrollBar = true;
            }
         }
      }
      else
      {
         this._scrollBar = MovieClip(value);
      }
      this.invalidate();
      if(this._scrollBar == null)
      {
         return;
      }
      if(this._scrollBar.setScrollProperties != null)
      {
         this._scrollBar.addEventListener("scroll",this,"handleScroll");
      }
      else
      {
         this._scrollBar.addEventListener("change",this,"handleScroll");
      }
      this._scrollBar.focusTarget = this;
      this._scrollBar.tabEnabled = false;
      this.updateScrollBar();
   }
   function get rowHeight()
   {
      return this._rowHeight;
   }
   function set rowHeight(value)
   {
      if(value == 0)
      {
         value = null;
      }
      this._rowHeight = value;
      this.invalidate();
   }
   function get scrollPosition()
   {
      return this._scrollPosition;
   }
   function set scrollPosition(value)
   {
      value = Math.max(0,Math.min(this._dataProvider.length - this.totalRenderers,Math.round(value)));
      if(this._scrollPosition == value)
      {
         return;
      }
      this._scrollPosition = value;
      this.invalidateData();
      this.updateScrollBar();
   }
   function get selectedIndex()
   {
      return this._selectedIndex;
   }
   function set selectedIndex(value)
   {
      if(value == this._selectedIndex)
      {
         return;
      }
      var renderer = this.getRendererAt(this._selectedIndex);
      if(renderer != null)
      {
         renderer.selected = false;
      }
      super.selectedIndex = value;
      if(this.totalRenderers == 0)
      {
         return;
      }
      renderer = this.getRendererAt(this._selectedIndex);
      if(renderer != null)
      {
         renderer.selected = true;
      }
      else
      {
         this.scrollToIndex(this._selectedIndex);
      }
   }
   function get disabled()
   {
      return this._disabled;
   }
   function set disabled(value)
   {
      super.disabled = value;
      if(this.initialized)
      {
         this.setState();
      }
   }
   function scrollToIndex(index)
   {
      if(this.totalRenderers == 0)
      {
         return undefined;
      }
      if(index >= this._scrollPosition && index < this._scrollPosition + this.totalRenderers)
      {
         this.scrollPosition = index;
      }
      else if(index < this._scrollPosition)
      {
         this.scrollPosition = index;
      }
      else
      {
         this.scrollPosition = index - (this.totalRenderers - 1);
      }
   }
   function get rowCount()
   {
      return this.totalRenderers;
   }
   function set rowCount(value)
   {
      var h = this._rowHeight;
      if(h == null)
      {
         var item = this.renderers[0];
         if(item == null)
         {
            item = this.createItemRenderer(0);
            if(item == null)
            {
               return;
            }
            h = item._height;
            item.removeMovieClip();
         }
         else
         {
            h = item.height;
         }
         if(h == null || h == 0)
         {
            return;
         }
      }
      this.height = h * value + this.margin * 2 + this.paddingTop + this.paddingBottom;
   }
   function invalidateData()
   {
      this._scrollPosition = Math.min(Math.max(0,this._dataProvider.length - this.totalRenderers),this._scrollPosition);
      this.selectedIndex = Math.min(this._dataProvider.length - 1,this._selectedIndex);
      this._dataProvider.requestItemRange(this._scrollPosition,Math.min(this._dataProvider.length - 1,this._scrollPosition + this.totalRenderers - 1),this,"populateData");
   }
   function handleInput(details, pathToFocus)
   {
      if(pathToFocus == null)
      {
         pathToFocus = [];
      }
      var renderer = this.getRendererAt(this._selectedIndex);
      if(renderer != null && renderer.handleInput != null)
      {
         var handled = renderer.handleInput(details,pathToFocus.slice(1));
         if(handled)
         {
            return true;
         }
      }
      var keyPress = details.value == "keyDown";
      switch(details.navEquivalent)
      {
         case gfx.ui.NavigationCode.UP:
            if(this._selectedIndex > 0)
            {
               if(keyPress)
               {
                  this.selectedIndex = this.selectedIndex - 1;
               }
               return true;
            }
            if(this.wrapping == "stick")
            {
               return true;
            }
            if(this.wrapping == "wrap")
            {
               if(keyPress)
               {
                  this.selectedIndex = this._dataProvider.length - 1;
               }
               return true;
            }
            return false;
            break;
         case gfx.ui.NavigationCode.DOWN:
            if(this._selectedIndex < this._dataProvider.length - 1)
            {
               if(keyPress)
               {
                  this.selectedIndex = this.selectedIndex + 1;
               }
               return true;
            }
            if(this.wrapping == "stick")
            {
               return true;
            }
            if(this.wrapping == "wrap")
            {
               if(keyPress)
               {
                  this.selectedIndex = 0;
               }
               return true;
            }
            return false;
            break;
         case gfx.ui.NavigationCode.END:
            if(!keyPress)
            {
               this.selectedIndex = this._dataProvider.length - 1;
            }
            return true;
         case gfx.ui.NavigationCode.HOME:
            if(!keyPress)
            {
               this.selectedIndex = 0;
            }
            return true;
         case gfx.ui.NavigationCode.PAGE_UP:
            if(keyPress)
            {
               this.selectedIndex = Math.max(0,this._selectedIndex - this.totalRenderers);
            }
            return true;
         case gfx.ui.NavigationCode.PAGE_DOWN:
            if(keyPress)
            {
               this.selectedIndex = Math.min(this._dataProvider.length - 1,this._selectedIndex + this.totalRenderers);
            }
            return true;
         default:
            return false;
      }
   }
   function get availableWidth()
   {
      return !this.autoScrollBar ? this.__width : this.__width - this._scrollBar._width;
   }
   function toString()
   {
      return "[Scaleform ScrollingList " + this._name + "]";
   }
   function configUI()
   {
      super.configUI();
      if(this.inspectableScrollBar != "")
      {
         this.scrollBar = this.inspectableScrollBar;
         this.inspectableScrollBar = null;
      }
   }
   function draw()
   {
      if(this.sizeIsInvalid)
      {
         this._width = this.__width;
         this._height = this.__height;
      }
      if(this.externalRenderers)
      {
         this.totalRenderers = this.renderers.length;
      }
      else
      {
         this.container._xscale = 10000 / this._xscale;
         this.container._yscale = 10000 / this._yscale;
         var h = this._rowHeight;
         if(h == null)
         {
            var temp = this.createItemRenderer(99);
            h = temp._height;
            temp.removeMovieClip();
         }
         var vertPadding = this.margin * 2 + this.paddingTop + this.paddingBottom;
         this.totalRenderers = Math.max(0,(this.__height - vertPadding + 0.05) / h >> 0);
         this.drawRenderers(this.totalRenderers);
         this.drawLayout(this.availableWidth,h);
      }
      this.updateScrollBar();
      this.invalidateData();
      this.setState();
      super.draw();
   }
   function drawLayout(rendererWidth, rendererHeight)
   {
      var horizPadding = this.paddingLeft + this.paddingRight + this.margin * 2;
      rendererWidth -= horizPadding;
      var i = 0;
      while(i < this.renderers.length)
      {
         this.renderers[i]._x = this.margin + this.paddingLeft;
         this.renderers[i]._y = i * rendererHeight + this.margin + this.paddingTop;
         this.renderers[i].setSize(rendererWidth,rendererHeight);
         i++;
      }
      this.drawScrollBar();
   }
   function drawScrollBar()
   {
      if(!this.autoScrollBar)
      {
         return undefined;
      }
      this._scrollBar._x = this.__width - this._scrollBar._width - this.margin;
      this._scrollBar._y = this.margin;
      this._scrollBar.height = this.__height - this.margin * 2;
   }
   function changeFocus()
   {
      super.changeFocus();
      this.setState();
   }
   function populateData(data)
   {
      var i = 0;
      while(i < this.renderers.length)
      {
         var renderer = this.renderers[i];
         var index = this._scrollPosition + i;
         this.renderers[i].setListData(index,this.itemToLabel(data[i]),this._selectedIndex == index);
         renderer.setData(data[i]);
         i++;
      }
      this.updateScrollBar();
   }
   function handleScroll(event)
   {
      var newPosition = event.target.position;
      if(isNaN(newPosition))
      {
         return undefined;
      }
      this.scrollPosition = newPosition;
   }
   function updateScrollBar()
   {
      var max = Math.max(0,this.dataProvider.length - this.totalRenderers);
      if(this._scrollBar.setScrollProperties != null)
      {
         this._scrollBar.setScrollProperties(this.totalRenderers * this.thumbSizeFactor,0,max);
      }
      else
      {
         this._scrollBar.minimum = 0;
         this._scrollBar.maximum = max;
      }
      this._scrollBar.position = this._scrollPosition;
   }
   function getRendererAt(index)
   {
      return this.renderers[index - this._scrollPosition];
   }
   function scrollWheel(delta)
   {
      if(this._disabled)
      {
         return undefined;
      }
      this.scrollPosition = this._scrollPosition - delta;
   }
   function setState()
   {
      this.tabEnabled = this.focusEnabled = !this._disabled;
      this.gotoAndPlay(!this._disabled ? (!this._focused ? "default" : "focused") : "disabled");
      if(this._scrollBar)
      {
         this._scrollBar.disabled = this._disabled;
      }
      var i = 0;
      while(i < this.renderers.length)
      {
         this.renderers[i].disabled = this._disabled;
         i++;
      }
   }
}
