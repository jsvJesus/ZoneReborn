package ui.components
{
   import com.dvalimona.components.*;
   import communication.*;
   import flash.display.DisplayObjectContainer;
   import flash.events.*;
   import flash.text.*;
   
   public class GoldPanel extends Component
   {
      public var box:HBox;
      
      protected var balanceLabel:BoldButton;
      
      protected var balanceValue:BoldButton;
      
      protected var balanceButton:BoldButton;
      
      public function GoldPanel(parent:DisplayObjectContainer = null, xpos:Number = 0, ypos:Number = 0)
      {
         this.visible = false;
         Gold.core.addEventListener(Gold.UPDATED,this.onGoldUpdated);
         super(parent,xpos,ypos);
      }
      
      protected function onGoldUpdated(event:Event) : void
      {
         invalidate();
      }
      
      override protected function addChildren() : void
      {
         super.addChildren();
         this.box = new HBox(this);
         this.box.spacing = 1;
         this.box.backgroundColor = 0;
         this.box.alignment = HBox.MIDDLE;
         this.box.horizontalAlign = HBox.LEFT;
         this.box.backgroundAlpha = 0.2;
         this.box.visible = true;
         this.balanceLabel = new BoldButton(this.box);
         this.balanceLabel.$ = "extendedGUI.GoldPanel.balanceGold";
         this.balanceLabel.mouseEnabled = false;
         this.balanceLabel.mouseChildren = false;
         this.balanceLabel.have_background = true;
         this.balanceValue = new BoldButton(this.box);
         this.balanceValue.autoWidth = true;
         this.balanceValue.label = String(Gold.Value);
         this.balanceValue.have_background = true;
         this.balanceValue.align = Label.RIGHT;
         this.balanceValue.labelComponent.align = TextFormatAlign.RIGHT;
         this.balanceValue.labelUpColor = 12700012;
         this.balanceValue.upColorAlpha = 1;
         this.balanceValue.mouseEnabled = false;
         this.balanceValue.mouseChildren = false;
         this.balanceButton = new BoldButton(this.box);
         this.balanceButton.labelOverColor = 12700012;
         this.balanceButton.labelUpColor = 5553663;
         this.balanceButton.have_background = true;
         this.balanceButton.$ = "extendedGUI.GoldPanel.buyGold";
         this.balanceButton.addEventListener(MouseEvent.CLICK,this.onBuyClick);
      }
      
      protected function onBuyClick(event:MouseEvent) : void
      {
         Api.call(Api.OPEN_URL,[{"url":"https://www.stalker.so/kabinet/getgold"}]);
      }
      
      public function get valueWidth() : Number
      {
         return this.balanceValue.textField.textWidth;
      }
      
      override public function draw() : void
      {
         super.draw();
         this.balanceValue.label = String(Gold.Value);
         this.balanceValue.invalidate();
         this.box.invalidate();
         this.balanceLabel.height = this.height;
         this.balanceValue.height = this.height;
         this.box.height = this.height;
         this.balanceButton.height = this.height;
      }
   }
}

