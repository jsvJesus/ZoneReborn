class com.scaleform.MapInfoItemRender extends gfx.controls.ListItemRenderer
{
   var data;
   var mapName;
   var mapNameLocalized;
   var players;
   var maxPlayers;
   var status;
   var icon;
   function MapInfoItemRender()
   {
      super();
   }
   function setData(data)
   {
      this.data = data;
      if(data)
      {
         this.enabled = true;
         this._visible = true;
         this.mapName._visible = true;
         this.mapNameLocalized._visible = true;
         this.players._visible = true;
         this.maxPlayers._visible = true;
         this.status._visible = true;
         this.icon._visible = true;
      }
      else
      {
         this.enabled = false;
         this._visible = true;
         this.mapName._visible = false;
         this.mapNameLocalized._visible = false;
         this.players._visible = false;
         this.maxPlayers._visible = false;
         this.status._visible = false;
         this.icon._visible = false;
      }
      this.updateData();
   }
   function updateData()
   {
      this.mapName.tf.text = this.data.mapName;
      this.mapNameLocalized.tf.text = this.data.mapNameLocalized;
      this.players.tf.text = this.data.players;
      this.maxPlayers.tf.text = "(" + this.data.maxPlayers + ")";
      this.icon.gotoAndStop(Number(this.data.mode) + 1);
      this.status.gotoAndStop(Number(this.data.isPrivate) + 1);
      this.gotoAndPlay(1);
   }
}
