object NetworkEventForm: TNetworkEventForm
  Left = 333
  Top = 298
  Width = 496
  Height = 290
  Caption = 'Network Event'
  Color = clBtnFace
  Constraints.MaxHeight = 290
  Constraints.MinHeight = 290
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnShow = FormShow
  DesignSize = (
    488
    256)
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 48
    Top = 40
    Width = 41
    Height = 13
    Caption = 'Process:'
  end
  object Label2: TLabel
    Left = 64
    Top = 16
    Width = 25
    Height = 13
    Caption = 'User:'
  end
  object Label3: TLabel
    Left = 24
    Top = 88
    Width = 69
    Height = 13
    Caption = 'Local address:'
  end
  object Label4: TLabel
    Left = 8
    Top = 112
    Width = 80
    Height = 13
    Caption = 'Remote address:'
  end
  object Label5: TLabel
    Left = 48
    Top = 64
    Width = 42
    Height = 13
    Caption = 'Protocol:'
  end
  object Label6: TLabel
    Left = 195
    Top = 64
    Width = 45
    Height = 13
    Caption = 'Direction:'
  end
  object Process: TEdit
    Left = 96
    Top = 37
    Width = 382
    Height = 21
    Anchors = [akLeft, akTop, akRight]
    ReadOnly = True
    TabOrder = 0
    Text = 'Process'
  end
  object User: TEdit
    Left = 96
    Top = 13
    Width = 382
    Height = 21
    Anchors = [akLeft, akTop, akRight]
    ReadOnly = True
    TabOrder = 1
    Text = 'Edit1'
  end
  object LocalAddress: TEdit
    Left = 96
    Top = 85
    Width = 382
    Height = 21
    Anchors = [akLeft, akTop, akRight]
    ReadOnly = True
    TabOrder = 2
    Text = 'Edit1'
  end
  object RemoteAddress: TEdit
    Left = 96
    Top = 109
    Width = 382
    Height = 21
    Anchors = [akLeft, akTop, akRight]
    ReadOnly = True
    TabOrder = 3
    Text = 'Edit1'
  end
  object Allow: TButton
    Left = 317
    Top = 224
    Width = 75
    Height = 25
    Anchors = [akTop, akRight]
    Caption = 'Allow'
    ModalResult = 1
    TabOrder = 4
  end
  object Block: TButton
    Left = 397
    Top = 224
    Width = 75
    Height = 25
    Anchors = [akTop, akRight]
    Caption = 'Block'
    ModalResult = 2
    TabOrder = 5
  end
  object Options: TRadioGroup
    Left = 8
    Top = 137
    Width = 470
    Height = 81
    Anchors = [akLeft, akTop, akRight]
    Caption = 'Options'
    ItemIndex = 1
    Items.Strings = (
      'One time action'
      
        'Always apply the selected action to combination [process]:[remot' +
        'e port]'
      'Always apply the selected action to this process')
    TabOrder = 6
  end
  object Protocol: TEdit
    Left = 96
    Top = 61
    Width = 89
    Height = 21
    ReadOnly = True
    TabOrder = 7
    Text = 'Edit1'
  end
  object Direction: TEdit
    Left = 243
    Top = 61
    Width = 126
    Height = 21
    ReadOnly = True
    TabOrder = 8
    Text = 'Direction'
  end
end
