class gfx.controls.ListItemRenderer extends gfx.controls.Button
{
   var owner;
   var index;
   var data;
   var focusTarget;
   var selectable = true;
   var enableInitCallback = false;
   function ListItemRenderer()
   {
      super();
      this.soundMap = {};
   }
   function get selected()
   {
      return this._selected;
   }
   function set selected(value)
   {
      super.selected = value;
      var _loc3_ = false;
      var _loc4_ = undefined;
      if(value && this.owner)
      {
         _loc3_ = this.owner.focused > 0;
         if(!_loc3_)
         {
            _loc4_ = this.owner.focusTarget;
         }
         if(_loc4_ && _loc4_.focused)
         {
            _loc3_ = true;
         }
      }
      this.displayFocus = value && _loc3_;
   }
   function setListData(index, label, selected)
   {
      this.index = index;
      if(label == null)
      {
         this.label = "Empty";
      }
      else
      {
         this.label = label;
      }
      this.state = "up";
      this.selected = selected;
   }
   function setData(data)
   {
      this.data = data;
   }
   function toString()
   {
      return "[Scaleform ListItemRenderer " + this._name + "]";
   }
   function configUI()
   {
      super.configUI();
      this.focusTarget = this.owner;
   }
}
