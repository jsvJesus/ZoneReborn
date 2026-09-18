package com.dvalimona.components
{
   import flash.display.*;
   import flash.events.*;
   import logging.*;
   
   public class RadioButton extends Component
   {
      protected static var buttons:Array;
      
      protected var _back:Sprite;
      
      protected var _button:Sprite;
      
      protected var _selected:Boolean = false;
      
      protected var _label:Label;
      
      protected var _labelText:String = "";
      
      protected var _groupName:String = "defaultRadioGroup";
      
      public function RadioButton(parent:DisplayObjectContainer = null, xpos:Number = 0, ypos:Number = 0, label:String = "", checked:Boolean = false, defaultHandler:Function = null)
      {
         try
         {
            RadioButton.addButton(this);
            this._selected = checked;
            this._labelText = label;
            super(parent,xpos,ypos);
            if(defaultHandler != null)
            {
               addEventListener(MouseEvent.CLICK,defaultHandler);
            }
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
      }
      
      protected static function addButton(rb:RadioButton) : void
      {
         try
         {
            if(buttons == null)
            {
               buttons = new Array();
            }
            buttons.push(rb);
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
      }
      
      protected static function clear(rb:RadioButton) : void
      {
         var i:uint = 0;
         try
         {
            for(i = 0; i < buttons.length; i++)
            {
               if(buttons[i] != rb && buttons[i].groupName == rb.groupName)
               {
                  buttons[i].selected = false;
               }
            }
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
      }
      
      override protected function init() : void
      {
         try
         {
            super.init();
            buttonMode = true;
            useHandCursor = true;
            addEventListener(MouseEvent.CLICK,this.onClick,false,1);
            this.selected = this._selected;
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
      }
      
      override protected function addChildren() : void
      {
         try
         {
            this._back = new Sprite();
            addChild(this._back);
            this._button = new Sprite();
            this._button.visible = false;
            addChild(this._button);
            this._label = new Label(this,0,0,this._labelText);
            this._label.size = 22;
            this.draw();
            mouseChildren = false;
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
      }
      
      override public function draw() : void
      {
         try
         {
            super.draw();
            this._back.graphics.clear();
            this._back.graphics.beginFill(Style.BACKGROUND);
            this._back.graphics.drawCircle(5,5,5);
            this._back.graphics.endFill();
            this._button.graphics.clear();
            this._button.graphics.beginFill(Style.BUTTON_UP);
            this._button.graphics.drawCircle(5,5,3);
            this._label.x = 12;
            this._label.y = (10 - this._label.height) / 2;
            this._label.text = this._labelText;
            this._label.draw();
            _width = this._label.width + 12;
            _height = 10;
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
      }
      
      protected function onClick(event:MouseEvent) : void
      {
         try
         {
            this.selected = true;
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
      }
      
      public function set selected(s:Boolean) : void
      {
         try
         {
            this._selected = s;
            this._button.visible = this._selected;
            if(this._selected)
            {
               RadioButton.clear(this);
            }
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
      }
      
      public function get selected() : Boolean
      {
         try
         {
            return this._selected;
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
         return false;
      }
      
      public function set label(str:String) : void
      {
         try
         {
            this._labelText = str;
            invalidate();
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
      }
      
      public function get label() : String
      {
         try
         {
            return this._labelText;
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
         return "761982763";
      }
      
      public function get groupName() : String
      {
         return this._groupName;
      }
      
      public function set groupName(value:String) : void
      {
         try
         {
            this._groupName = value;
         }
         catch(error:Error)
         {
            Logger.LogToChannel(Logger.ERROR,"CharButton.updateContent error:",error);
         }
      }
   }
}

