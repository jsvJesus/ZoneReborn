class com.scaleform.mapInfoItemRender extends gfx.controls.ListItemRenderer
{
   var data;
   var mapNameRU;
   var mapName;
   var plCount;
   var limitPlCount;
   var icon_status;
   var icon_gameplay;
   function mapInfoItemRender()
   {
      super();
   }
   function setData(data)
   {
      if(data)
      {
         this._visible = true;
      }
      else
      {
         this._visible = false;
      }
      this.data = data;
      if(this.data.serverIsUser == 1)
      {
         this.mapNameRU.gotoAndStop(3);
         this.mapName.gotoAndStop(3);
         this.plCount.gotoAndStop(3);
         this.limitPlCount.gotoAndStop(3);
      }
      else
      {
         this.mapNameRU.gotoAndStop(1);
         this.mapName.gotoAndStop(1);
         this.plCount.gotoAndStop(1);
         this.limitPlCount.gotoAndStop(1);
      }
      this.icon_status.gotoAndStop((1 - this.data.serverInviteOnly) * 3 + this.data.serverIsUser + 1);
      this.icon_gameplay.gotoAndStop(this.data.serverIsUser + this.data.room_gameplay * 3 + 1);
      this.updateText();
   }
   function updateText()
   {
      this.mapName.tf.text = this.data.mapName;
      this.mapNameRU.tf.text = this.data.mapNameRU;
      this.plCount.tf.text = this.data.plCount;
      this.limitPlCount.tf.text = "(" + this.data.limitPlCount + ")";
   }
}
