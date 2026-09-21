class com.scaleform.ModeInfoItemRender extends gfx.controls.ListItemRenderer
{
   var data;
   var icon;
   function ModeInfoItemRender()
   {
      super();
   }
   function setData(data)
   {
      this.data = data;
      if(data)
      {
         if(this.data.isEnabled)
         {
            this.enabled = true;
            this._alpha = 100;
            this._alpha = 100;
         }
         else
         {
            this.enabled = false;
            this._alpha = 25;
            this._alpha = 25;
         }
         this._visible = true;
         this.label._visible = true;
         this.icon._visible = true;
      }
      else
      {
         this.enabled = false;
         this._visible = true;
         this.label._visible = false;
         this.icon._visible = false;
      }
      this.updateData();
   }
   function updateData()
   {
      this.label.tf.text = this.data.label;
      this.icon.gotoAndStop(Number(this.data.value) + 1);
   }
}
