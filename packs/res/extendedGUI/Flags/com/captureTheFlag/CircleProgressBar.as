package com.captureTheFlag
{
   import flash.display.Graphics;
   import flash.display.Shape;
   import flash.display.Sprite;
   import flash.events.Event;
   
   public class CircleProgressBar extends Sprite
   {
      private var _percent:Number;
      
      private var _width:int;
      
      private var _height:int;
      
      private var _shape:Shape;
      
      private var _canvas:Graphics;
      
      private var _step:Number;
      
      private var _color:uint;
      
      private var _thickness:int;
      
      private var scale:Number;
      
      private var halfWidth:int;
      
      private var halfHeight:int;
      
      private var halfWidthWithoutThickness:int;
      
      private var halfHeightWidthoutThickness:int;
      
      public function CircleProgressBar(width:int = 100, height:int = 100, thickness:int = 30, color:uint = 6750054, step:Number = 0.2, value:Number = 0.7)
      {
         super();
         this._width = width;
         this._height = height;
         this._percent = value;
         this._step = step;
         this._shape = new Shape();
         this._canvas = this._shape.graphics;
         this._color = color;
         addChild(this._shape);
         if(thickness >= 0 && thickness <= int(width / 2))
         {
            this._thickness = thickness;
         }
         else
         {
            this._thickness = int(width / 2);
         }
         this.halfWidth = int(this._width / 2);
         this.halfHeight = int(this._width / 2);
         this.halfWidthWithoutThickness = int((this._width - 2 * this._thickness) / 2);
         this.halfHeightWidthoutThickness = int((this._width - 2 * this._thickness) / 2);
         this.scale = this._height / this._width;
         this.update();
      }
      
      private function update() : void
      {
         this._canvas.clear();
         this._canvas.beginFill(this._color);
         var endAngle:Number = this._percent * 6.283185307179586;
         this._canvas.moveTo(this.halfWidth + Math.cos(endAngle) * this.halfWidth,this.halfHeight + Math.sin(endAngle) * this.halfHeight);
         for(var angle1:Number = endAngle; angle1 > 0; angle1 -= this._step)
         {
            this._canvas.lineTo(this.halfWidth + Math.cos(angle1) * this.halfWidth,this.halfHeight + Math.sin(angle1) * this.halfHeight);
         }
         this._canvas.lineTo(this.halfWidth + Math.cos(0) * this.halfWidth,this.halfHeight + Math.sin(0) * this.halfHeight);
         for(var angle2:Number = 0; angle2 < endAngle; angle2 += this._step)
         {
            this._canvas.lineTo(this.halfWidth + Math.cos(angle2) * this.halfWidthWithoutThickness,this.halfHeight + Math.sin(angle2) * this.halfHeightWidthoutThickness);
         }
         this._canvas.lineTo(this.halfWidth + Math.cos(endAngle) * this.halfWidthWithoutThickness,this.halfHeight + Math.sin(endAngle) * this.halfHeightWidthoutThickness);
         this._canvas.endFill();
         this._shape.scaleY = this.scale;
         dispatchEvent(new Event(Event.CHANGE));
      }
      
      public function set value(value:Number) : void
      {
         if(value < 0 || value > 100)
         {
            return;
         }
         this._percent = value / 100;
         this.update();
      }
      
      public function get value() : Number
      {
         return this._percent;
      }
      
      public function set color(value:Number) : void
      {
         this._color = value;
         this.update();
      }
   }
}

