using System;
using System.Drawing;
using System.Drawing.Design;
using System.Windows.Forms;
using System.Windows.Forms.Design;

namespace Be.Windows.Forms.Design
{
	/// <summary>
	/// Display only fixed-piched fonts
	/// </summary>
	internal class HexFontEditor : FontEditor
	{
		object value;

		/// <summary>
		/// Initializes an instance of HexFontEditor class.
		/// </summary>
		public HexFontEditor()
		{
		}

		/// <summary>
		/// Edits the value
		/// </summary>
		public override object EditValue(System.ComponentModel.ITypeDescriptorContext context, IServiceProvider provider, object value)
		{
			this.value = value;
			if (provider != null)
			{
				IWindowsFormsEditorService service1 = (IWindowsFormsEditorService) provider.GetService(typeof(IWindowsFormsEditorService));
				if (service1 != null)
				{
					FontDialog fontDialog = new FontDialog();
					fontDialog.ShowApply = false;
					fontDialog.ShowColor = false;
					fontDialog.AllowVerticalFonts = false;
					fontDialog.AllowScriptChange = false;
					fontDialog.FixedPitchOnly = true;
					fontDialog.ShowEffects = false;
					fontDialog.ShowHelp = false;

					Font font = value as Font;
					if(font != null)
					{
						fontDialog.Font = font;
					}
					if (fontDialog.ShowDialog() == DialogResult.OK)
					{
						this.value = fontDialog.Font;
					}

					fontDialog.Dispose();
				}
			}

			value = this.value;
			this.value = null;
			return value;

		}

		public override UITypeEditorEditStyle GetEditStyle(System.ComponentModel.ITypeDescriptorContext context)
		{
			return UITypeEditorEditStyle.Modal;
		}


	}
}
