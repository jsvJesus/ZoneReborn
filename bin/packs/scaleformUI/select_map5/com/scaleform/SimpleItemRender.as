class com.scaleform.SimpleItemRender extends gfx.controls.ListItemRenderer
{
   var data;
   var owner;
   var background;
   var hitarea;
   static var WIDTH = {};
   function SimpleItemRender()
   {
      super();
   }
   function setData(data)
   {
      this.data = data;
      if(com.scaleform.SimpleItemRender.WIDTH[this.owner._name])
      {
         var targetWidth = com.scaleform.SimpleItemRender.WIDTH[this.owner._name];
         this.background._width = targetWidth;
         this.hitarea._width = targetWidth;
         this.label.tf._width = targetWidth - this.label._x * 2;
      }
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
      }
      else
      {
         this.enabled = false;
         this._visible = true;
         this.label._visible = false;
      }
      this.updateText();
   }
   function updateText()
   {
      if(this.data.isEnabled == false)
      {
         this._alpha = 50;
      }
      else
      {
         this._alpha = 100;
      }
      this.label.tf.text = this.data.label;
   }
}
