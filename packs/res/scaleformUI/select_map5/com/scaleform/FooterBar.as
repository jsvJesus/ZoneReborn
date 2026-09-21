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
      this.endTimeGame = mapdata.endTimeGame;
      this.window_map.tf.text = mapdata.mapNameRU;
      this.window_map.icons.gotoAndStop(mapdata.mapName);
      this.window_userCount.tf_players.text = mapdata.plCount;
      this.window_userCount.tf_players_limit.text = "/" + mapdata.limitPlCount;
      this.window_gameplay.gp_icon.gotoAndStop(2 - mapdata.serverIsUser + mapdata.room_gameplay * 3 + 1);
      var ar = ["Убить Всех","Командный бой","А","Б","С"];
      var strGamePlay = ar[mapdata.room_gameplay];
      this.window_gameplay.tf.text = strGamePlay;
   }
   function printData()
   {
   }
   function timerUpdate0()
   {
      var my_date = new Date();
      var timeLost = this.endTimeGame - my_date.getTime() / 1000;
      if(timeLost < 1 or !timeLost)
      {
         this.window_time.tf.text = "00:00";
      }
      else
      {
         var sec = Math.floor(timeLost);
         var minut = Math.floor(sec / 60);
         var hours = Math.floor(minut / 60);
         minut = String(minut % 60);
         if(minut.length < 2)
         {
            minut = "0" + minut;
         }
         hours = String(hours % 24);
         if(hours.length < 2)
         {
            hours = "0" + hours;
         }
         var t = ".";
         if(sec % 2)
         {
            t = ":";
         }
         this.window_time.tf.text = hours + t + minut;
      }
   }
}
