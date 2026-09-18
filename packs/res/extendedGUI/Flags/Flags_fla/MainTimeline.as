package Flags_fla
{
   import adobe.utils.*;
   import com.greensock.TweenMax;
   import com.greensock.easing.*;
   import flash.accessibility.*;
   import flash.desktop.*;
   import flash.display.*;
   import flash.errors.*;
   import flash.events.*;
   import flash.external.*;
   import flash.filters.*;
   import flash.geom.*;
   import flash.globalization.*;
   import flash.media.*;
   import flash.net.*;
   import flash.net.drm.*;
   import flash.printing.*;
   import flash.profiler.*;
   import flash.sampler.*;
   import flash.sensors.*;
   import flash.system.*;
   import flash.text.*;
   import flash.text.engine.*;
   import flash.text.ime.*;
   import flash.ui.*;
   import flash.utils.*;
   import flash.xml.*;
   import scaleform.clik.controls.Button1;
   
   public dynamic class MainTimeline extends MovieClip
   {
      public var base:MainBaseInterace;
      
      public var compact_state:Boolean;
      
      public var compactBtn:Button1;
      
      public var w:*;
      
      public var h:Number;
      
      public function MainTimeline()
      {
         super();
      }
      
      public function set_money(Obj:*) : *
      {
         var flag_dat:Object = Obj.flag;
         var guard_dat:Object = Obj.guardian;
         this.base.MainFlags.setMoneyIcon(flag_dat.cost,flag_dat.base_money,flag_dat.is_gold);
         this.base.BuyGuards.setMoneyIcon(guard_dat.cost,guard_dat.base_money,guard_dat.is_gold);
      }
      
      public function validNotification() : *
      {
         if(this.compact_state)
         {
            this.compactBtn._State = "notification";
         }
         else
         {
            this.compactBtn._State = "";
         }
         this.compactBtn.setState("up");
      }
      
      public function stopResize() : *
      {
         this.compactBtn.removeEventListener(MouseEvent.CLICK,this.onCompactClick);
      }
      
      public function startResize() : *
      {
         this.compactBtn.addEventListener(MouseEvent.CLICK,this.onCompactClick);
      }
      
      public function onCompactClick(e:MouseEvent) : *
      {
         this.compact_state = !this.compact_state;
         ExternalInterface.call("compact_state",{"value":this.compact_state});
         this.base.alpha = 0;
      }
      
      public function onResize(arg1:*) : *
      {
         var scale:Number = NaN;
         var w:* = undefined;
         var h:Number = NaN;
         if(arg1.compact_state != null)
         {
            this.compact_state = arg1.compact_state;
         }
         if(this.compact_state)
         {
            this.base.visible = false;
            this.compactBtn.label = "+";
         }
         else
         {
            this.compactBtn.label = "-";
            this.base.visible = true;
            this.validNotification();
            scale = this.base.width / this.base.height;
            w = scale * arg1.height;
            if(w > arg1.width)
            {
               this.base.width = arg1.width;
               this.base.height = 1 / scale * arg1.width;
            }
            else
            {
               this.base.height = arg1.height;
               this.base.width = arg1.height * scale;
            }
            this.base.y = (Object(root).height - stage.stageHeight) / 2 + stage.stageHeight * arg1.corner[1] + 3;
            this.base.x = (Object(root).width - stage.stageWidth) / 2 + stage.stageWidth * arg1.corner[0] + arg1.width / 2 - this.base.width / 2;
            TweenMax.to(this.base,0.2,{
               "alpha":1,
               "delay":0,
               "ease":Expo.easeIn
            });
         }
         this.compactBtn.y = (Object(root).height - stage.stageHeight) / 2 + stage.stageHeight * arg1.corner[1] + 7;
         this.compactBtn.x = (Object(root).width - stage.stageWidth) / 2 + stage.stageWidth * arg1.corner[0] + arg1.width / 2 - this.compactBtn.width / 2;
      }
   }
}

