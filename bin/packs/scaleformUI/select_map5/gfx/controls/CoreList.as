class gfx.controls.CoreList extends gfx.core.UIComponent
{
   var renderers;
   var container;
   var _dataProvider;
   var dispatchEvent;
   var _labelFunction;
   var owner;
   var inspectableRendererInstanceName;
   var _itemRenderer = "ListItemRenderer";
   var _selectedIndex = -1;
   var _labelField = "label";
   var externalRenderers = false;
   var deferredScrollIndex = -1;
   function CoreList()
   {
      super();
      this.renderers = [];
      this.dataProvider = [];
      this.container = this.createEmptyMovieClip("container",1);
      this.container.scale9Grid = new flash.geom.Rectangle(0,0,1,1);
      this.tabEnabled = this.focusEnabled = true;
   }
   function get itemRenderer()
   {
      return this._itemRenderer;
   }
   function set itemRenderer(value)
   {
      if(value == this._itemRenderer || value == "")
      {
         return;
      }
      this._itemRenderer = value;
      this.resetRenderers();
      this.invalidate();
   }
   function get dataProvider()
   {
      return this._dataProvider;
   }
   function set dataProvider(value)
   {
      if(this._dataProvider == value)
      {
         return;
      }
      if(this._dataProvider != null)
      {
         this._dataProvider.removeEventListener("change",this,"onDataChange");
      }
      this._dataProvider = value;
      if(this._dataProvider == null)
      {
         return;
      }
      if(value instanceof Array && !value.isDataProvider)
      {
         gfx.data.DataProvider.initialize(this._dataProvider);
      }
      else if(this._dataProvider.initialize != null)
      {
         this._dataProvider.initialize(this);
      }
      this._dataProvider.addEventListener("change",this,"onDataChange");
      this.invalidate();
   }
   function get selectedIndex()
   {
      return this._selectedIndex;
   }
   function set selectedIndex(value)
   {
      var lastIndex = this._selectedIndex;
      this._selectedIndex = value;
      this.dispatchEvent({type:"change",index:this._selectedIndex,lastIndex:lastIndex});
   }
   function scrollToIndex(index)
   {
   }
   function get labelField()
   {
      return this._labelField;
   }
   function set labelField(value)
   {
      this._labelField = value;
      this.invalidateData();
   }
   function get labelFunction()
   {
      return this._labelFunction;
   }
   function set labelFunction(value)
   {
      this._labelFunction = value;
      this.invalidateData();
   }
   function itemToLabel(item)
   {
      if(item == null)
      {
         return "";
      }
      if(this._labelFunction != null)
      {
         return this._labelFunction(item);
      }
      if(this._labelField != null && item[this._labelField] != null)
      {
         return item[this._labelField];
      }
      return item.toString();
   }
   function invalidateData()
   {
   }
   function get availableWidth()
   {
      return this.__width;
   }
   function get availableHeight()
   {
      return this.__height;
   }
   function setRendererList(value)
   {
      if(this.externalRenderers)
      {
         var i = 0;
         while(i < this.renderers.length)
         {
            var clip = this.renderers[i];
            clip.owner = null;
            clip.removeEventListener("click",this,"handleItemClick");
            clip.removeEventListener("rollOver",this,"dispatchItemEvent");
            clip.removeEventListener("rollOut",this,"dispatchItemEvent");
            clip.removeEventListener("press",this,"dispatchItemEvent");
            clip.removeEventListener("doubleClick",this,"dispatchItemEvent");
            Mouse.removeListener(clip);
            i++;
         }
      }
      else
      {
         this.resetRenderers();
      }
      this.externalRenderers = value != null;
      if(this.externalRenderers)
      {
         this.renderers = value;
      }
      this.invalidate();
   }
   function get rendererInstanceName()
   {
      return null;
   }
   function set rendererInstanceName(value)
   {
      if(value == null || value == "")
      {
         return;
      }
      var i = 0;
      var newRenderers = [];
      while(true)
      {
         i++;
         var clip = this._parent[value + i];
         if(clip == null && i > 0)
         {
            break;
         }
         if(clip != null)
         {
            this.setUpRenderer(clip);
            Mouse.addListener(clip);
            clip.scrollWheel = function(delta)
            {
               this.owner.scrollWheel(delta);
            };
            newRenderers.push(clip);
         }
      }
      if(newRenderers.length == 0)
      {
         newRenderers = null;
      }
      this.setRendererList(newRenderers);
   }
   function toString()
   {
      return "[Scaleform CoreList " + this._name + "]";
   }
   function configUI()
   {
      super.configUI();
      if(this._selectedIndex > -1)
      {
         this.deferredScrollIndex = this._selectedIndex;
      }
      if(this.inspectableRendererInstanceName != "")
      {
         this.rendererInstanceName = this.inspectableRendererInstanceName;
      }
      Mouse.addListener(this);
   }
   function createItemRenderer(index)
   {
      var clip = this.container.attachMovie(this._itemRenderer,"renderer" + index,index);
      if(clip == null)
      {
         return null;
      }
      this.setUpRenderer(clip);
      return clip;
   }
   function setUpRenderer(clip)
   {
      clip.owner = this;
      clip.tabEnabled = false;
      clip.doubleClickEnabled = true;
      clip.addEventListener("press",this,"dispatchItemEvent");
      clip.addEventListener("click",this,"handleItemClick");
      clip.addEventListener("doubleClick",this,"dispatchItemEvent");
      clip.addEventListener("rollOver",this,"dispatchItemEvent");
      clip.addEventListener("rollOut",this,"dispatchItemEvent");
   }
   function createItemRenderers(startIndex, endIndex)
   {
      var list = [];
      var i = startIndex;
      while(i <= endIndex)
      {
         list.push(this.createItemRenderer[i]);
         i++;
      }
      return list;
   }
   function draw()
   {
      if(this.deferredScrollIndex != -1)
      {
         this.scrollToIndex(this.deferredScrollIndex);
         this.deferredScrollIndex = -1;
      }
   }
   function drawRenderers(totalRenderers)
   {
      while(this.renderers.length > totalRenderers)
      {
         this.renderers.pop().removeMovieClip();
      }
      while(this.renderers.length < totalRenderers)
      {
         this.renderers.push(this.createItemRenderer(this.renderers.length));
      }
   }
   function getRendererAt(index)
   {
      return this.renderers[index];
   }
   function resetRenderers()
   {
      while(this.renderers.length > 0)
      {
         this.renderers.pop().removeMovieClip();
      }
   }
   function drawLayout(rendererWidth, rendererHeight)
   {
   }
   function onDataChange(event)
   {
      this.invalidateData();
   }
   function dispatchItemEvent(event)
   {
      var type;
      switch(event.type)
      {
         case "press":
            type = "itemPress";
            break;
         case "click":
            type = "itemClick";
            break;
         case "rollOver":
            type = "itemRollOver";
            break;
         case "rollOut":
            type = "itemRollOut";
            break;
         case "doubleClick":
            type = "itemDoubleClick";
            break;
         default:
            return undefined;
      }
      var newEvent = {target:this,type:type,item:event.target.data,renderer:event.target,index:event.target.index,mouseIndex:event.mouseIndex};
      this.dispatchEvent(newEvent);
   }
   function handleItemClick(event)
   {
      var index = event.target.index;
      if(isNaN(index))
      {
         return undefined;
      }
      this.selectedIndex = index;
      this.dispatchItemEvent(event);
   }
}
