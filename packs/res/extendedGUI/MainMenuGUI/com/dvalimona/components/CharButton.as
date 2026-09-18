package com.dvalimona.components
{
   import com.greensock.*;
   import com.greensock.easing.*;
   import communication.*;
   import flash.display.*;
   import flash.events.*;
   import flash.utils.*;
   import lang.*;
   import logging.*;
   import ui.components.*;
   
   public class CharButton extends RadioButton
   {
      private static const AnimationTime:Number = 0.3;
      
      private var _character:Character;
      
      protected var backLeftShift:Number = 50;
      
      protected var rightBorder:uint = 10;
      
      private var _fixedHeight:uint;
      
      private var _fixedWidth:uint;
      
      private var box:HBoxLine;
      
      private var deleteChar:PushButton;
      
      private var restoreChar:PushButton;
      
      private var restoreLabel:LabelShadowed;
      
      public function CharButton(parent:DisplayObjectContainer = null, xpos:Number = 0, ypos:Number = 0, label:String = "", checked:Boolean = false, defaultHandler:Function = null)
      {
         super(parent,xpos,ypos,label,checked,defaultHandler);
      }
      
      public function set character(char:Character) : void
      {
         this._character = char;
         this.label = Boolean(this._character) && Boolean(this._character.name) ? this._character.name : "[ПУСТО]";
         if(this.allowRestore)
         {
            this._label.color = 8947848;
         }
      }
      
      public function get character() : Character
      {
         return this._character;
      }
      
      protected function get allowDelete() : Boolean
      {
         return Boolean(this._character) && Boolean(this._character.deletionRemainingTime) ? this._character.deletionRemainingTime < 0 : false;
      }
      
      protected function get allowRestore() : Boolean
      {
         return Boolean(this._character) && Boolean(this._character.deletionRemainingTime) ? this._character.deletionRemainingTime >= 0 : false;
      }
      
      public function get fixedHeight() : uint
      {
         return this._fixedHeight;
      }
      
      public function set fixedHeight(value:uint) : void
      {
         this._fixedHeight = value;
         invalidate();
      }
      
      public function get fixedWidth() : uint
      {
         return this._fixedWidth;
      }
      
      public function set fixedWidth(value:uint) : void
      {
         this._fixedWidth = value;
         invalidate();
      }
      
      override protected function init() : void
      {
         super.init();
         buttonMode = true;
         useHandCursor = true;
         addEventListener(MouseEvent.CLICK,this.onClick,false);
         this.selected = _selected;
      }
      
      override protected function addChildren() : void
      {
         _back = new Sprite();
         _back.scaleY = 0;
         _back.scaleX = 0;
         this.addChild(_back);
         this.box = new HBoxLine(this,0,0);
         this.box.debug = false;
         this.box.left.alignment = HBox.MIDDLE;
         this.box.right.alignment = HBox.MIDDLE;
         _label = new LabelShadowed();
         (_label as LabelShadowed).shadowColor = 0;
         (_label as LabelShadowed).shadowAlpha = 0.9;
         (_label as LabelShadowed).shadowSize = 1;
         _label.size = 22;
         _label.autoSize = true;
         _label.text = "";
         _label.paddingLeft = 10;
         this.box.left.addChild(_label);
         this.deleteChar = new MenuButton();
         this.deleteChar.$ = "extendedGUI.CharWindow.deleteChar";
         this.deleteChar.underline = false;
         this.deleteChar.size = 18;
         this.deleteChar.align = Label.RIGHT;
         this.deleteChar.paddingRight = 10;
         this.deleteChar.addEventListener(MouseEvent.CLICK,this.deleteCharHandler);
         this.restoreChar = new MenuButton();
         this.restoreChar.y = 4;
         this.restoreChar.$ = "extendedGUI.CharWindow.restoreChar";
         this.restoreChar.underline = false;
         this.restoreChar.size = 18;
         this.restoreChar.align = Label.RIGHT;
         this.restoreChar.paddingRight = 10;
         this.restoreChar.addEventListener(MouseEvent.CLICK,this.restoreCharHandler);
         this.restoreLabel = new LabelShadowed();
         this.restoreLabel.shadowColor = 0;
         this.restoreLabel.color = 8947848;
         this.restoreLabel.shadowAlpha = 0.9;
         this.restoreLabel.shadowSize = 1;
         this.restoreLabel.size = 16;
         this.restoreLabel.autoSize = true;
         this.restoreLabel.paddingLeft = 10;
         this.restoreLabel.paddingTop = 2;
         this.draw();
      }
      
      protected function restoreCharHandler(event:MouseEvent) : void
      {
         this.restoreChar.enabled = false;
         Logger.LogToChannel(Logger.DEBUG,this,"restoreCharHandler");
         Character.RestoreLastSelectedChar();
      }
      
      protected function deleteCharHandler(event:MouseEvent) : void
      {
         this.deleteChar.enabled = false;
         Logger.LogToChannel(Logger.DEBUG,this,"deleteCharHandler");
         Character.DeleteLastSelectedChar();
      }
      
      override protected function onClick(event:MouseEvent) : void
      {
         Logger.LogToChannel(Logger.DEBUG,this,"onClick:",selected);
         if(!selected)
         {
            this.selected = true;
            setTimeout(this.setOwnerCharAsCurrent,30,this.character.id);
         }
      }
      
      private function setOwnerCharAsCurrent(target_id:Number) : void
      {
         Character.selectById(target_id);
      }
      
      override public function set selected(s:Boolean) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"CharButton, selected:",_selected);
         _selected = s;
         if(_selected)
         {
            RadioButton.clear(this);
            TweenMax.to(this._back,AnimationTime,{
               "scaleY":1,
               "scaleX":1,
               "ease":Expo.easeIn
            });
            if(this.allowDelete)
            {
               this.box.right.addChild(this.deleteChar);
               this.deleteChar.alpha = 0;
               TweenMax.to(this.deleteChar,AnimationTime,{
                  "alpha":1,
                  "ease":Expo.easeIn
               });
            }
            if(this.allowRestore)
            {
               this.restoreChar.alpha = 0;
               this.restoreLabel.alpha = 0;
               this.box.right.addChild(this.restoreChar);
               this.box.left.addChild(this.restoreLabel);
               this.restoreLabel.text = this.getRestoreLabelText();
               TweenMax.to(this.restoreChar,AnimationTime,{
                  "alpha":1,
                  "ease":Expo.easeIn
               });
               TweenMax.to(this.restoreLabel,AnimationTime,{
                  "alpha":1,
                  "delay":0,
                  "ease":Expo.easeIn
               });
            }
         }
         else
         {
            TweenMax.to(this._back,AnimationTime,{
               "scaleY":0,
               "scaleX":0,
               "ease":Expo.easeIn
            });
            if(this.allowDelete)
            {
               if(this.contains(this.deleteChar))
               {
                  TweenMax.to(this.deleteChar,AnimationTime,{
                     "alpha":0,
                     "ease":Expo.easeIn,
                     "onComplete":function():void
                     {
                        box.right.removeChild(deleteChar);
                        box.right.draw();
                     }
                  });
               }
            }
            if(this.allowRestore)
            {
               if(this.contains(this.restoreChar))
               {
                  TweenMax.to(this.restoreChar,AnimationTime,{
                     "alpha":0,
                     "ease":Expo.easeIn,
                     "onComplete":function():void
                     {
                        box.right.removeChild(restoreChar);
                        box.right.draw();
                     }
                  });
                  TweenMax.to(this.restoreLabel,AnimationTime,{
                     "alpha":0,
                     "ease":Expo.easeIn,
                     "onComplete":function():void
                     {
                        box.left.removeChild(restoreLabel);
                        box.right.draw();
                     }
                  });
               }
            }
         }
      }
      
      private function getRestoreLabelText() : String
      {
         var result:String = "";
         return Locale.getById("extendedGUI.CharWindow.deleteAfter") + " " + this.getTimeToDelete();
      }
      
      private function getTimeToDelete() : String
      {
         var days:uint = 0;
         var hours:uint = 0;
         var minutes:uint = 0;
         var result:String = "";
         var timeToRemain:uint = uint(this.character.deletionRemainingTime * 1);
         days = timeToRemain / (60 * 60 * 24);
         hours = timeToRemain / (60 * 60);
         minutes = timeToRemain / 60;
         if(days >= 1)
         {
            result = days + Locale.getById("extendedGUI.CharWindow.days");
         }
         else if(hours >= 1)
         {
            result = hours + Locale.getById("extendedGUI.CharWindow.hours");
         }
         else
         {
            result = minutes + Locale.getById("extendedGUI.CharWindow.minutes");
         }
         Logger.LogToChannel(Logger.DEBUG,"CharButton.getTimeToDelete:",days,hours,minutes);
         return result;
      }
      
      override public function draw() : void
      {
         _label.text = _labelText;
         _back.graphics.clear();
         _back.graphics.beginFill(0,0.5);
         _back.graphics.drawRect(0,-height / 2,width,height);
         _back.graphics.endFill();
         _back.y = height / 2;
         this.box.setSize(_width,_height);
      }
   }
}

