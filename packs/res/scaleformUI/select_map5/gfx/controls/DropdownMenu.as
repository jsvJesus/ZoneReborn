class gfx.controls.DropdownMenu extends gfx.controls.Button
{
   var inspectableDropdown;
   var inspectableItemRenderer;
   var inspectableScrollBar;
   var _dataProvider;
   var _labelFunction;
   var onMouseDown;
   var addEventListener;
   var selectedItem;
   var dispatchEvent;
   var isOpen = false;
   var direction = "down";
   var _dropdownWidth = -1;
   var _rowCount = 5;
   var _labelField = "label";
   var _selectedIndex = -1;
   var margin = 1;
   var paddingTop = 0;
   var paddingBottom = 0;
   var paddingLeft = 0;
   var paddingRight = 0;
   var thumbOffsetTop = 0;
   var thumbOffsetBottom = 0;
   var thumbSizeFactor = 1;
   var offsetX = 0;
   var offsetY = 0;
   var extent = 0;
   var _dropdown = null;
   function DropdownMenu()
   {
      super();
      this.dataProvider = [];
   }
   function get dropdown()
   {
      return this._dropdown;
   }
   function set dropdown(value)
   {
      this.inspectableDropdown = value;
      if(this.initialized)
      {
         this.createDropDown();
      }
   }
   function get itemRenderer()
   {
      return this._dropdown.itemRenderer;
   }
   function set itemRenderer(value)
   {
      this.inspectableItemRenderer = value;
      if(this._dropdown)
      {
         this._dropdown.itemRenderer = this.inspectableItemRenderer;
      }
   }
   function get scrollBar()
   {
      return this._dropdown.scrollBar;
   }
   function set scrollBar(value)
   {
      this.inspectableScrollBar = value;
      if(this._dropdown)
      {
         this._dropdown.scrollBar = this.inspectableScrollBar;
      }
   }
   function get dropdownWidth()
   {
      return this._dropdownWidth;
   }
   function set dropdownWidth(value)
   {
      this._dropdownWidth = value;
   }
   function get rowCount()
   {
      return this._rowCount;
   }
   function set rowCount(value)
   {
      this._rowCount = value;
      if(this._dropdown != null && this._dropdown.hasOwnProperty("rowCount"))
      {
         this._dropdown.rowCount = value;
      }
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
      gfx.data.DataProvider.initialize(this._dataProvider);
      this._dataProvider.addEventListener("change",this,"onDataChange");
      if(this._dropdown != null)
      {
         this._dropdown.dataProvider = this._dataProvider;
      }
      this.selectedIndex = 0;
      this.updateSelectedItem();
   }
   function get selectedIndex()
   {
      return this._selectedIndex;
   }
   function set selectedIndex(value)
   {
      this._selectedIndex = value;
      if(this._dropdown != null)
      {
         this._dropdown.selectedIndex = value;
      }
      this.updateSelectedItem();
   }
   function get labelField()
   {
      return this._labelField;
   }
   function set labelField(value)
   {
      this._labelField = value;
      if(this._dropdown != null)
      {
         this._dropdown.labelField = this._labelField;
      }
      this.updateLabel();
   }
   function get labelFunction()
   {
      return this._labelFunction;
   }
   function set labelFunction(value)
   {
      this._labelFunction = value;
      if(this._dropdown != null)
      {
         this._dropdown.labelFunction = this._labelFunction;
      }
      this.updateLabel();
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
   function open()
   {
      if(!this._dropdown)
      {
         this.createDropDown();
         if(!this._dropdown)
         {
            this.isOpen = false;
            return undefined;
         }
      }
      this.openImpl();
   }
   function close()
   {
      if(!this.isOpen)
      {
         return undefined;
      }
      this.isOpen = false;
      if(this._dropdown == null)
      {
         return undefined;
      }
      this._dropdown.visible = false;
      this.onMouseDown = null;
      this.selected = false;
   }
   function invalidateData()
   {
      this._dataProvider.requestItemAt(this._selectedIndex,this,"populateText");
   }
   function setSize(width, height)
   {
      super.setSize(width,height);
      if(this._dropdown != null && this._dropdownWidth == -1)
      {
         this._dropdown.width = this.__width;
      }
   }
   function handleInput(details, pathToFocus)
   {
      var handled;
      if(this._dropdown != null && this.isOpen)
      {
         handled = this._dropdown.handleInput(details);
         if(handled)
         {
            return true;
         }
      }
      handled = super.handleInput(details,pathToFocus);
      var keyPress = details.value == "keyDown";
      var _loc0_ = null;
      if((_loc0_ = details.navEquivalent) === gfx.ui.NavigationCode.ESCAPE)
      {
         if(this.isOpen)
         {
            if(keyPress)
            {
               this.close();
            }
            return true;
         }
      }
      return handled;
   }
   function removeMovieClip()
   {
      this._dropdown.removeMovieClip();
      super.removeMovieClip();
   }
   function toString()
   {
      return "[Scaleform DropdownMenu " + this._name + "]";
   }
   function createDropDown()
   {
      if(this._dropdown == this.inspectableDropdown)
      {
         return undefined;
      }
      if(this._dropdown != null)
      {
         this._dropdown.removeMovieClip();
      }
      if(typeof this.inspectableDropdown == "string")
      {
         this._dropdown = MovieClip(this._parent[this.inspectableDropdown]);
         if(this._dropdown == null)
         {
            this._dropdown = gfx.managers.PopUpManager.createPopUp(this,this.inspectableDropdown.toString(),{itemRenderer:this.inspectableItemRenderer,paddingTop:this.paddingTop,paddingBottom:this.paddingBottom,paddingLeft:this.paddingLeft,paddingRight:this.paddingRight,thumbOffsetTop:this.thumbOffsetTop,thumbOffsetBottom:this.thumbOffsetBottom,thumbSizeFactor:this.thumbSizeFactor,margin:this.margin});
            this._dropdown.scrollBar = this.inspectableScrollBar;
         }
      }
      else if(this.inspectableDropdown instanceof MovieClip)
      {
         this._dropdown = MovieClip(this._dropdown);
      }
      if(this._dropdown == null)
      {
         return undefined;
      }
      this._dropdown.addEventListener("itemClick",this,"handleChange");
      this._dropdown.labelField = this._labelField;
      this._dropdown.labelFunction = this._labelFunction;
      this._dropdown.dataProvider = this._dataProvider;
      this._dropdown.selectedIndex = this._selectedIndex;
      this._dropdown.visible = false;
      if(this._dropdown.wrapping != null)
      {
         this._dropdown.wrapping = "stick";
      }
      if(this._dropdown.autoRowCount != null)
      {
         this._dropdown.autoRowCount = true;
      }
      if(this._dropdown.rowCount != null)
      {
         this._dropdown.rowCount = Math.min(this._dataProvider.length,this._rowCount);
      }
      if(this._dropdownWidth != 0)
      {
         this._dropdown.width = this._dropdownWidth != -1 ? this._dropdownWidth : this.__width + this.extent;
      }
      this._dropdown.focusTarget = this;
   }
   function openImpl()
   {
      if(this.isOpen)
      {
         return undefined;
      }
      this.isOpen = true;
      if(this._dropdownWidth != 0)
      {
         this._dropdown.width = this._dropdownWidth != -1 ? this._dropdownWidth : this.__width + this.extent;
      }
      if(this._dropdown.rowCount != undefined)
      {
         this._dropdown.rowCount = Math.min(this._dataProvider.length,this._rowCount);
      }
      this._dropdown.validateNow();
      this.onMouseDown = this.handleStageClick;
      this.selected = true;
      this._dropdown.selectedIndex = this._selectedIndex;
      this._dropdown.scrollToIndex(this._selectedIndex);
      var oX = this.offsetX * (100 / this._xscale);
      var oY = this.offsetY * (100 / this._yscale);
      gfx.managers.PopUpManager.movePopUp(this,MovieClip(this._dropdown),this,oX,this.direction != "down" ? - this._dropdown.height - oY : this.__height + oY);
      this._dropdown.visible = true;
   }
   function configUI()
   {
      super.configUI();
      this.addEventListener("click",this,"toggleMenu");
      this.toggle = false;
   }
   function draw()
   {
      super.draw();
   }
   function changeFocus()
   {
      super.changeFocus();
      if(this.isOpen && this._dropdown != null)
      {
         this.close();
      }
   }
   function updateSelectedItem()
   {
      this.invalidateData();
   }
   function updateLabel()
   {
      this.label = this.itemToLabel(this.selectedItem);
   }
   function populateText(item)
   {
      this.selectedItem = item;
      this.updateLabel();
      this.dispatchEvent({type:"change",index:this._selectedIndex,data:item});
   }
   function handleChange(event)
   {
      var index = this._dropdown.selectedIndex;
      this._selectedIndex = index;
      this.close();
      this.updateSelectedItem();
   }
   function onDataChange(event)
   {
      this.updateSelectedItem();
   }
   function toggleMenu(event)
   {
      !!this._selected ? this.close() : this.open();
   }
   function handleStageClick(event)
   {
      if(this.hitTest(_root._xmouse,_root._ymouse,true))
      {
         return undefined;
      }
      this.close();
   }
   function hitTest(x, y, shapeFlag)
   {
      return super.hitTest(x,y,shapeFlag) || (!(this._dropdown && this.isOpen) ? false : this._dropdown.hitTest(x,y,shapeFlag));
   }
}
