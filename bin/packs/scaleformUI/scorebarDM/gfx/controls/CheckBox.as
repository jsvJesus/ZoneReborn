class gfx.controls.CheckBox extends gfx.controls.Button
{
   function CheckBox()
   {
      super();
   }
   function toString()
   {
      return "[Scaleform CheckBox " + this._name + "]";
   }
   function configUI()
   {
      super.configUI();
      this.toggle = true;
   }
}
