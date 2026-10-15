class com.scaleform.dmScopePlayerItemRender extends gfx.controls.ListItemRenderer
{
   var statNumber;
   var plname;
   var kills;
   var deads;
   var score;
   function dmScopePlayerItemRender()
   {
      super();
   }
   function setData(data)
   {
      if(!data)
      {
         this.statNumber.text = "";
         this.plname.text = "";
         this.kills.text = "";
         this.deads.text = "";
         this.score.text = "";
         return undefined;
      }
      var _loc4_ = "#A8FB9C";
      var _loc5_ = "#D6D6D6";
      var _loc2_ = _loc5_;
      if(data.isMe != 0)
      {
         _loc2_ = _loc4_;
      }
      this.statNumber.text = data.statNumber;
      this.plname.htmlText = "<FONT FACE=\"Bender\" SIZE=\"15\" COLOR=\"" + _loc2_ + "\" LETTERSPACING=\"0.000000\" KERNING=\"1\"> " + data.plname + "</FONT>";
      this.kills.htmlText = "<FONT FACE=\"Bender\" SIZE=\"15\" COLOR=\"" + _loc2_ + "\" LETTERSPACING=\"0.000000\" KERNING=\"1\"> " + data.kills + "</FONT>";
      this.deads.htmlText = "<FONT FACE=\"Bender\" SIZE=\"15\" COLOR=\"" + _loc2_ + "\" LETTERSPACING=\"0.000000\" KERNING=\"1\"> " + data.deads + "</FONT>";
      this.score.htmlText = "<FONT FACE=\"Bender\" SIZE=\"15\" COLOR=\"" + _loc2_ + "\" LETTERSPACING=\"0.000000\" KERNING=\"1\"> " + data.score + "</FONT>";
   }
}
