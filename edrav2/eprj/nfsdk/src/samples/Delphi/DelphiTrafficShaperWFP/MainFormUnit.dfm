object MainForm: TMainForm
  Left = 293
  Top = 286
  BorderStyle = bsDialog
  Caption = 'Traffic Shaper'
  ClientHeight = 212
  ClientWidth = 318
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 24
    Top = 64
    Width = 70
    Height = 13
    Caption = 'Process name:'
  end
  object Label2: TLabel
    Left = 10
    Top = 96
    Width = 85
    Height = 13
    Caption = 'Bytes per second:'
  end
  object Start: TButton
    Left = 16
    Top = 8
    Width = 88
    Height = 25
    Caption = 'Start'
    TabOrder = 0
    OnClick = StartClick
  end
  object Stop: TButton
    Left = 112
    Top = 8
    Width = 88
    Height = 25
    Caption = 'Stop'
    Enabled = False
    TabOrder = 1
    OnClick = StopClick
  end
  object ProcessName: TEdit
    Left = 101
    Top = 61
    Width = 196
    Height = 21
    TabOrder = 2
    Text = 'firefox.exe'
  end
  object Limit: TEdit
    Left = 101
    Top = 93
    Width = 196
    Height = 21
    TabOrder = 3
    Text = '10000'
  end
  object GroupBox1: TGroupBox
    Left = 8
    Top = 120
    Width = 297
    Height = 81
    Caption = 'Statistics'
    TabOrder = 4
    object Label3: TLabel
      Left = 10
      Top = 24
      Width = 40
      Height = 13
      Caption = 'In bytes:'
    end
    object InBytesLabel: TLabel
      Left = 63
      Top = 24
      Width = 6
      Height = 13
      Caption = '0'
    end
    object Label4: TLabel
      Left = 154
      Top = 24
      Width = 44
      Height = 13
      Caption = 'In speed:'
    end
    object InSpeedLabel: TLabel
      Left = 207
      Top = 24
      Width = 6
      Height = 13
      Caption = '0'
    end
    object Label5: TLabel
      Left = 10
      Top = 48
      Width = 48
      Height = 13
      Caption = 'Out bytes:'
    end
    object OutBytesLabel: TLabel
      Left = 63
      Top = 48
      Width = 6
      Height = 13
      Caption = '0'
    end
    object Label7: TLabel
      Left = 154
      Top = 48
      Width = 52
      Height = 13
      Caption = 'Out speed:'
    end
    object OutSpeedLabel: TLabel
      Left = 207
      Top = 48
      Width = 6
      Height = 13
      Caption = '0'
    end
  end
  object Timer1: TTimer
    OnTimer = Timer1Timer
    Left = 248
    Top = 24
  end
end
