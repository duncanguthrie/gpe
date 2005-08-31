// Client.cs -
//
// Author: Rachel Hestilow  <hestilow@nullenvoid.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of version 2 of the Lesser GNU General 
// Public License as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.


namespace GConf
{
	using System;
	using System.Collections;
	using System.Runtime.InteropServices;
	
	public class Client : ClientBase
	{
		IntPtr Raw = IntPtr.Zero;
		Hashtable dirs = new Hashtable ();
		Hashtable callbacks = new Hashtable ();

		[DllImport("gconf-2")]
		static extern IntPtr gconf_client_get_default ();

		public Client ()
		{
			Initialize ();
			Raw = gconf_client_get_default ();
		}

		internal Client (IntPtr raw)
		{
			Initialize ();
			Raw = raw;
		}

		[DllImport("gconf-2")]
		static extern void gconf_client_set (IntPtr client, string key, IntPtr val, out IntPtr err);

		internal override void SetValue (string key, Value val)
		{
			IntPtr err;
			gconf_client_set (Raw, key, val.Handle, out err);
			if (err != IntPtr.Zero)
				throw new GLib.GException (err);
		}

		[DllImport("gconf-2")]
		static extern IntPtr gconf_client_get (IntPtr client, string key, out IntPtr err);

		public override object Get (string key)
		{
			if (key == null)
				throw new ArgumentNullException ("key");

			IntPtr err;
			IntPtr raw = gconf_client_get (Raw, key, out err);
			if (err != IntPtr.Zero)
				throw new GLib.GException (err);
			
			if (raw == IntPtr.Zero)
				throw new NoSuchKeyException (key);

			Value val = new Value (raw);
			return val.Get ();
		}

		[DllImport("gconf-2")]
		static extern uint gconf_client_add_dir (IntPtr client, string dir, int preload, out IntPtr err);

		void AddDir (string dir)
		{
			IntPtr err;
			
			if (dirs.Contains (dir))
				return;

			dirs.Add (dir, 1);
			gconf_client_add_dir (Raw, dir, 0, out err);
			if (err != IntPtr.Zero)
				throw new GLib.GException (err);
		}
		
		[DllImport("gconf-2")]
		static extern uint gconf_client_notify_add (IntPtr client, string dir, NotifyFuncNative func, IntPtr user_data, IntPtr destroy_notify, out IntPtr err);
		
		public void AddNotify (string dir, NotifyEventHandler notify)
		{
			IntPtr err;

			AddDir (dir);
			uint cnxn = gconf_client_notify_add (Raw, dir, NotifyWrapper.Wrap (notify), IntPtr.Zero, IntPtr.Zero, out err);
			if (err != IntPtr.Zero)
				throw new GLib.GException (err);
			callbacks.Add (notify, cnxn);
		}

		[DllImport("gconf-2")]
		static extern void gconf_client_notify_remove (IntPtr client, uint cnxn);

		public void RemoveNotify (string dir, NotifyEventHandler notify)
		{
			if (!callbacks.Contains (notify))
				return;
			
			uint cnxn = (uint) callbacks[notify];
			gconf_client_notify_remove (Raw, cnxn);
			callbacks.Remove (notify);
		}

		[DllImport("gconf-2")]
		static extern void gconf_client_suggest_sync (IntPtr client, out IntPtr err);

		public void SuggestSync ()
		{
			IntPtr err;
			gconf_client_suggest_sync (Raw, out err);
			if (err != IntPtr.Zero)
				throw new GLib.GException (err);
		}
	}
}
