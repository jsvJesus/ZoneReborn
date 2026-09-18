package com.colorPicker
{
   import flash.display.MovieClip;
   
   public class ColorList extends MovieClip
   {
      private var _colors:Object = new Object();
      
      private var X:Number = 0;
      
      private var Y:Number = 0;
      
      public function ColorList()
      {
         super();
      }
      
      public function setColor(value:uint) : *
      {
         var i:* = undefined;
         if(this._colors[value.toString(16)] != null)
         {
            for(i in this._colors)
            {
               this._colors[i].selected = false;
            }
            this._colors[value.toString(16)].selected = true;
         }
      }
      
      public function Draw(arr:Array) : *
      {
         var str:String = null;
         var t:Number = 1;
         for(var i:* = 0; i < arr.length; i++)
         {
            str = arr[i].toString(16);
            this._colors[str] = new ColorItem();
            this._colors[str].color = arr[i];
            this._colors[str].addEventListener(ColorEvent.SELECT,this.onClick);
            this._colors[str].x = this.X;
            this._colors[str].y = this.Y;
            addChild(this._colors[str]);
            this.X += 13;
            if(t >= 16)
            {
               t = 0;
               this.Y += 13;
               this.X = 0;
            }
            t++;
         }
      }
      
      internal function onClick(e:ColorEvent) : *
      {
         dispatchEvent(new ColorEvent(ColorEvent.SELECT,e.color));
         this.setColor(e.color);
      }
   }
}

