object Form1: TForm1
  Left = 192
  Top = 125
  Width = 351
  Height = 186
  Caption = 'PFHttpAddRequestHeader'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Name: TLabel
    Left = 24
    Top = 56
    Width = 67
    Height = 13
    Caption = 'Header name:'
  end
  object Value: TLabel
    Left = 24
    Top = 88
    Width = 67
    Height = 13
    Caption = 'Header value:'
  end
  object Start: TButton
    Left = 16
    Top = 8
    Width = 75
    Height = 25
    Caption = 'Start'
    TabOrder = 0
    OnClick = StartClick
  end
  object Stop: TButton
    Left = 104
    Top = 8
    Width = 75
    Height = 25
    Caption = 'Stop'
    Enabled = False
    TabOrder = 1
    OnClick = StopClick
  end
  object HeaderNameEdit: TEdit
    Left = 96
    Top = 51
    Width = 201
    Height = 21
    TabOrder = 2
    Text = 'Test-Header'
  end
  object HeaderValueEdit: TEdit
    Left = 96
    Top = 83
    Width = 201
    Height = 21
    TabOrder = 3
    Text = 'Test value'
  end
end
