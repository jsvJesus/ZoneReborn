class com.scaleform.FooterBar extends MovieClip
{
   var window_map;
   var interval;
   var endTimeGame;
   var window_userCount;
   var window_gameplay;
   var window_time;
   function FooterBar()
   {
      super();
   }
   function onLoad()
   {
      trace("FooterBar onLoad");
      this.window_map.tf.text = "";
      this.window_map.icons.gotoAndStop("dm_map15");
      this.interval = setInterval(mx.utils.Delegate.create(this,this.intCall),500);
   }
   function intCall()
   {
      this.timerUpdate0();
      clearInterval(this.interval);
      this.interval = setInterval(mx.utils.Delegate.create(this,this.intCall),500);
   }
   function setData(mapdata)
   {
      trace("foter setData:" + mapdata.mapNameRU);
      trace("foter setData this.endTimeGame:" + this.endTimeGame);
      this.endTimeGame = mapdata.endTimeGame;
      trace("foter setData2 this.endTimeGame:" + this.endTimeGame);
      this.window_map.tf.text = mapdata.mapNameRU;
      this.window_map.icons.gotoAndStop(mapdata.mapName);
      this.window_userCount.tf_players.text = mapdata.plCount;
      this.window_userCount.tf_players_limit.text = "/" + mapdata.limitPlCount;
      this.window_gameplay.gp_icon.gotoAndStop(2 - mapdata.serverIsUser + mapdata.room_gameplay * 3 + 1);
      var _loc3_ = ["Убить Всех","Командный бой","А","Б","С"];
      var _loc4_ = _loc3_[mapdata.room_gameplay];
      this.window_gameplay.tf.text = _loc4_;
   }
   function printData()
   {
      trace("foter printData this.endTimeGame:" + this.endTimeGame);
   }
   function timerUpdate0()
   {
      var _loc7_ = new Date();
      var _loc4_ = this.endTimeGame - _loc7_.getTime() / 1000;
      if(_loc4_ < 1 or !_loc4_)
      {
         this.window_time.tf.text = "00:00";
      }
      else
      {
         var _loc5_ = Math.floor(_loc4_);
         var _loc2_ = Math.floor(_loc5_ / 60);
         var _loc3_ = Math.floor(_loc2_ / 60);
         _loc2_ = String(_loc2_ % 60);
         if(_loc2_.length < 2)
         {
            _loc2_ = "0" + _loc2_;
         }
         _loc3_ = String(_loc3_ % 24);
         if(_loc3_.length < 2)
         {
            _loc3_ = "0" + _loc3_;
         }
         var _loc6_ = ".";
         if(_loc5_ % 2)
         {
            _loc6_ = ":";
         }
         this.window_time.tf.text = _loc3_ + _loc6_ + _loc2_;
      }
   }
}
