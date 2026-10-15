class gfx.controls.ListItemRenderer extends gfx.controls.Button
{
   var index;
   var data;
   var focusTarget;
   var owner;
   var selectable = true;
   function ListItemRenderer()
   {
      super();
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
