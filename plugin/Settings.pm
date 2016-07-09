package Plugins::CastBridge::Settings;

use strict;

use File::Spec::Functions;
use LWP::Simple;
use base qw(Slim::Web::Settings);
use XML::Simple;
use Data::Dumper;
use Slim::Utils::PluginManager;
use Slim::Utils::Prefs;
use Slim::Utils::Log;


my $prefs = preferences('plugin.castbridge');
my $log   = logger('plugin.castbridge');
my @xmlmain = qw(upnp_socket scan_interval scan_timeout log_limit);
my @xmldevice = qw(name mac buffer_dir buffer_limit sample_rate codecs flac_header enabled remove_count send_metadata volume_on_play send_coverart send_icy stream_pacing_size media_volume server);

sub name { 'PLUGIN_CASTBRIDGE' }

sub page { 'plugins/CastBridge/settings/basic.html' }
	
sub handler {
	my ($class, $client, $params, $callback, @args) = @_;

	my $update;
	
	require Plugins::CastBridge::Squeeze2cast;
	require Plugins::CastBridge::Plugin;
			
	if ($params->{ 'delconfig' }) {
				
		my $conf = Plugins::CastBridge::Squeeze2cast->configFile($class);
		unlink $conf;							
		$log->info("deleting configuration $conf");
		
		#okay, this is hacky, will change in the future, just don't want another indent layer :-(
		$params->{'saveSettings'} = 0;
		
		$update = 1;
	}
		
	if ($params->{ 'genconfig' }) {
	
		Plugins::CastBridge::Squeeze2cast->stop;
		waitEndHandler(\&genConfig, $class, $client, $params, $callback, 30, @args);
	
		return undef;
	}
	
	if ($params->{ 'cleanlog' }) {
				
		my $logfile = Plugins::CastBridge::Squeeze2cast->logFile($class);
		open my $fh, ">", $logfile;
		print $fh;
		close $fh;
		
		#okay, this is hacky, will change in the future, just don't want another indent layer :-(
		$params->{'saveSettings'} = 0;
	}
	
	if ($params->{'saveSettings'}) {

		$log->debug("save settings required");
		
		my @bool  = qw(autorun logging autosave eraselog);
		my @other = qw(output bin debugs opts);
		my $skipxml;
				
		for my $param (@bool) {
			
			my $val = $params->{ $param } ? 1 : 0;
			
			if ($val != $prefs->get($param)) {
					
				$prefs->set($param, $val);
				$update = 1;
					
			}
		}
		
		# check that the config file name has not changed first
		for my $param (@other) {
		
			if ($params->{ $param } ne $prefs->get($param)) {
			
				$prefs->set($param, $params->{ $param });
				$update = 1;
			}
		}
		
		if ($params->{ 'configfile' } ne $prefs->get('configfile')) {
		
			$prefs->set('configfile', $params->{ 'configfile' });
			$update = 1;
			$skipxml = 1;
		}	
		
		my $xmlconfig = readconfig($class, KeyAttr => 'device');
				
		# get XML player configuration if current device has changed in the list
		if ($xmlconfig and !$skipxml and ($params->{'seldevice'} eq $params->{'prevseldevice'})) {
		
			$log->info('Writing XML:', $params->{'seldevice'});
			for my $p (@xmlmain) {
				
				if ($params->{ $p } eq '') {
					delete $xmlconfig->{ $p };
				} else {
					$xmlconfig->{ $p } = $params->{ $p };
				}	
			}
			
			$log->info("current: ", $params->{'seldevice'}, "previous: ", $params->{'prevseldevice'});
			
			#save common parameters
			if ($params->{'seldevice'} eq '.common.') {
			
				for my $p (@xmldevice) {
					if ($params->{ $p } eq '') {
						delete $xmlconfig->{ common }->{ $p };
					} else {
						$xmlconfig->{ common }->{ $p } = $params->{ $p };
					}
				}	
				
			} else {
			
				if ($params->{'deldevice'}) {
					#delete current device	
					$log->info(@{$xmlconfig->{'device'}});
					@{$xmlconfig->{'device'}} = grep $_->{'udn'} ne $params->{'seldevice'}, @{$xmlconfig->{'device'}};
					$params->{'seldevice'} = '.common.';
					
				} else {
					# save player specific parameters
					$params->{'devices'} = \@{$xmlconfig->{'device'}};
					my $device = findUDN($params->{'seldevice'}, $params->{'devices'});
					
					for my $p (@xmldevice) {
						if ($params->{ $p } eq '') {
							delete $device->{ $p };
						} else {
							$device->{ $p } = $params->{ $p };
						}
					}	
					
				}			
			}	
			
			# get enabled status for all device, except the selected one (if any)
			foreach my $device (@{$xmlconfig->{'device'}}) {
				if ($device->{'udn'} ne $params->{'seldevice'}) {
					my $enabled = $params->{ 'enabled.'.$device->{ 'udn' } };
					$device->{'enabled'} = defined $enabled ? $enabled : 0;
				}	
			}
			
			$log->info("writing XML config");
			$log->debug(Dumper($xmlconfig));
			Plugins::CastBridge::Squeeze2cast->stop;
			waitEndHandler( sub { XMLout($xmlconfig, RootName => "squeeze2cast", NoSort => 1, NoAttr => 1, OutputFile => Plugins::CastBridge::Squeeze2cast->configFile($class)); }, 
							$class, $client, $params, $callback, 30, @args);
			$update = 1;
		}	
	}

	# something has been updated, XML array is up-to-date anyway, but need to write it
	if ($update) {

		$log->debug("updating");
				
		Plugins::CastBridge::Squeeze2cast->stop;
		waitEndHandler(undef, $class, $client, $params, $callback, 30, @args);
		
	#no update detected or first time looping
	} else {

		$log->debug("not updating");
		$class->handler2($client, $params, $callback, @args);		  
	}

	return undef;
}

sub waitEndHandler	{
	my ($func, $class, $client, $params, $callback, $wait, @args) = @_;
	
	if (Plugins::CastBridge::Squeeze2cast->alive()) {
		$log->debug('Waiting for squeeze2cast to end');
		$wait--;
		if ($wait) {
			Slim::Utils::Timers::setTimer($class, Time::HiRes::time() + 1, sub {
				waitEndHandler($func, $class, $client, $params, $callback, $wait, @args); });
		}		

	} else {
		if (defined $func) {
			$func->($class, $client, $params, $callback, @args);
		}
		else {
			if ($prefs->get('autorun')) {
				Plugins::CastBridge::Squeeze2cast->start
			}
	
			$class->handler2($client, $params, $callback, @args);		  
		}	
	}
}

sub genConfig {
	my ($class, $client, $params, $callback, @args) = @_;
	
	my $conf = Plugins::CastBridge::Squeeze2cast->configFile($class);
	Plugins::CastBridge::Squeeze2cast->start( "-i", $conf );
	waitEndHandler(undef, $class, $client, $params, $callback, 120, @args);
}	

sub handler2 {
	my ($class, $client, $params, $callback, @args) = @_;

	if ($prefs->get('autorun')) {

		$params->{'running'}  = Plugins::CastBridge::Squeeze2cast->alive;

	} else {

		$params->{'running'} = 0;
	}

	$params->{'binary'}   = Plugins::CastBridge::Squeeze2cast->bin;
	$params->{'binaries'} = [ Plugins::CastBridge::Squeeze2cast->binaries ];
	for my $param (qw(autorun output bin opts debugs logging configfile autosave eraselog)) {
		$params->{ $param } = $prefs->get($param);
	}
	
	$params->{'configpath'} = Slim::Utils::OSDetect::dirsFor('prefs');
	$params->{'arch'} = Slim::Utils::OSDetect::OS();
	
	my $xmlconfig = readconfig($class, KeyAttr => 'device');
		
	#load XML parameters from config file	
	if ($xmlconfig) {
	
		$params->{'devices'} = \@{$xmlconfig->{'device'}};
		unshift(@{$params->{'devices'}}, {'name' => '[default parameters]', 'udn' => '.common.'});
		
		$log->info("reading config: ", $params->{'seldevice'});
		$log->debug(Dumper($params->{'devices'}));
				
		#read global parameters
		for my $p (@xmlmain) {
			$params->{ $p } = $xmlconfig->{ $p };
			$log->debug("reading: ", $p, " ", $xmlconfig->{ $p });
		}
		
		# read either common parameters or device-specific
		if (!defined $params->{'seldevice'} or ($params->{'seldevice'} eq '.common.')) {
			$params->{'seldevice'} = '.common.';
			
			for my $p (@xmldevice) {
				$params->{ $p } = $xmlconfig->{common}->{ $p };
			}	
		} else {
			my $device = findUDN($params->{'seldevice'}, $params->{'devices'});
			
			for my $p (@xmldevice) {
				$params->{ $p } = $device->{ $p };
			}
		}
		$params->{'prevseldevice'} = $params->{'seldevice'};
		$params->{'xmlparams'} = 1;
		
	} else {
	
		$params->{'xmlparams'} = 0;
	}
	
	$callback->($client, $params, $class->SUPER::handler($client, $params), @args);
}

sub mergeprofile{
	my ($p1, $p2) = @_;
	
	foreach my $m (keys %$p2) {
		$p1->{ $m } = $p2-> { $m };
	}	
}


sub findUDN {
	my $udn = shift(@_);
	my $listpar = shift(@_);
	my @list = @{$listpar};
	
	while (@list) {
		my $p = pop @list;
		if ($p->{ 'udn' } eq $udn) { return $p; }
	}
	return undef;
}


sub readconfig {
	my ($class,@args) = @_;
	my $ret;
	
	my $file = Plugins::CastBridge::Squeeze2cast->configFile($class);
	if (-e $file) {
		$ret = XMLin($file, ForceArray => ['device'], KeepRoot => 0, NoAttr => 1, @args);
	}	
	return $ret;
}

1;
