package com
{
   import flash.display.MovieClip;
   import flash.events.Event;
   import flash.events.MouseEvent;
   import flash.text.TextField;
   import flash.utils.Timer;
   import flash.utils.setTimeout;
   
   public class NotificationBar extends MovieClip
   {
      public var forMask:Notif_Maska;
      
      public var nothi:MovieClip;
      
      public var background:MovieClip;
      
      public var VISIBLE_TIME:Number = 7000;
      
      public var Text1:TextField;
      
      protected var speed:int = 20;
      
      protected var N:Number = 0;
      
      protected var step:Number = 0;
      
      protected var stepH:Number = 0;
      
      protected var i:int = 0;
      
      public var timer:Timer = new Timer(this.VISIBLE_TIME);
      
      public function NotificationBar()
      {
         super();
         stage.addEventListener(Event.RESIZE,this.onResizeLog);
         this.visible = false;
         this.addEventListener(MouseEvent.CLICK,this.onMouseClick);
      }
      
      protected function onMouseClick(e:MouseEvent) : *
      {
         this.Hide();
      }
      
      protected function onResizeLog(e:Event) : *
      {
         this.x = Object(root).width / 2 - (this.Text1.width + 10) / 2;
      }
      
      public function Show(text:String) : *
      {
         this.Text1.height = 10;
         this.Text1.width = 650;
         this.Text1.htmlText = text;
         if(this.Text1.textWidth + 10 > stage.stageWidth / 2)
         {
            this.Text1.width = stage.stageWidth / 2;
         }
         else
         {
            this.Text1.width = this.Text1.textWidth + 10;
         }
         this.Text1.height = Math.round(this.Text1.textWidth / this.Text1.width) * (this.Text1.textHeight + 6);
         if(this.Text1.height == 0)
         {
            this.Text1.height = this.Text1.textHeight + 6;
         }
         this.background.height = this.Text1.height + 5;
         this.background.width = this.Text1.width + 100;
         this.alpha = 1;
         this.visible = true;
         this.nothi.visible = true;
         this.forMask.x = 0;
         this.forMask.width = 15;
         this.forMask.height = this.Text1.height;
         this.nothi.width = 15;
         this.nothi.x = 0;
         this.N = (this.background.width - this.forMask.width) / this.speed;
         this.step = this.nothi.width / this.N;
         this.i = 0;
         this.timer.stop();
         this.x = Object(root).width / 2 - (this.background.width + 10) / 2;
         this.removeEventListener(Event.ENTER_FRAME,this.onFrameHide);
         this.addEventListener(Event.ENTER_FRAME,this.onFrame);
      }
      
      protected function Hide() : *
      {
         this.i = 0;
         if(this.timer.currentCount >= 1)
         {
            this.nothi.visible = true;
            this.removeEventListener(Event.ENTER_FRAME,this.onFrame);
            this.addEventListener(Event.ENTER_FRAME,this.onFrameHide);
         }
      }
      
      protected function visibleOff(e:Event) : *
      {
         if(this.alpha > 0)
         {
            this.alpha -= 0.1;
         }
         else
         {
            this.removeEventListener(Event.ENTER_FRAME,this.visibleOff);
            this.visible = false;
         }
      }
      
      protected function visibleOn(e:Event) : *
      {
         if(this.alpha < 1)
         {
            this.alpha += 0.1;
         }
         else
         {
            this.removeEventListener(Event.ENTER_FRAME,this.visibleOn);
            this.visible = true;
         }
      }
      
      protected function onFrameHide(e:Event) : *
      {
         if(this.i < this.N)
         {
            ++this.i;
            this.forMask.width -= this.speed;
            if(this.nothi.width < 33)
            {
               this.nothi.width += this.step;
            }
            this.nothi.x = this.forMask.width + this.forMask.x - this.nothi.width;
         }
         else
         {
            removeEventListener(Event.ENTER_FRAME,this.onFrameHide);
            this.visible = false;
         }
      }
      
      protected function onFrame(e:Event) : *
      {
         if(this.i < this.N)
         {
            ++this.i;
            this.forMask.width += this.speed;
            if(this.nothi.width > 0)
            {
               this.nothi.width -= this.step;
            }
            this.nothi.x = this.forMask.width + this.forMask.x - this.nothi.width;
         }
         else
         {
            this.nothi.visible = false;
            removeEventListener(Event.ENTER_FRAME,this.onFrame);
            this.timer.start();
            setTimeout(this.Hide,this.VISIBLE_TIME);
         }
      }
   }
}

