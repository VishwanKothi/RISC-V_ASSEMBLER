
import type { Config } from "tailwindcss";

export default {
	darkMode: ["class"],
	content: [
		"./pages/**/*.{ts,tsx}",
		"./components/**/*.{ts,tsx}",
		"./app/**/*.{ts,tsx}",
		"./src/**/*.{ts,tsx}",
	],
	prefix: "",
	theme: {
		container: {
			center: true,
			padding: '2rem',
			screens: {
				'2xl': '1400px'
			}
		},
		extend: {
			colors: {
				border: 'hsl(var(--border))',
				input: 'hsl(var(--input))',
				ring: 'hsl(var(--ring))',
				background: 'hsl(var(--background))',
				foreground: 'hsl(var(--foreground))',
				primary: {
					DEFAULT: 'hsl(var(--primary))',
					foreground: 'hsl(var(--primary-foreground))'
				},
				secondary: {
					DEFAULT: 'hsl(var(--secondary))',
					foreground: 'hsl(var(--secondary-foreground))'
				},
				destructive: {
					DEFAULT: 'hsl(var(--destructive))',
					foreground: 'hsl(var(--destructive-foreground))'
				},
				muted: {
					DEFAULT: 'hsl(var(--muted))',
					foreground: 'hsl(var(--muted-foreground))'
				},
				accent: {
					DEFAULT: 'hsl(var(--accent))',
					foreground: 'hsl(var(--accent-foreground))'
				},
				popover: {
					DEFAULT: 'hsl(var(--popover))',
					foreground: 'hsl(var(--popover-foreground))'
				},
				card: {
					DEFAULT: 'hsl(var(--card))',
					foreground: 'hsl(var(--card-foreground))'
				},
				// Stage colors - Light Mode
				"fetch-light": "#E6F7FF", // Light blue
				"fetch-text-light": "#0066CC",
				"decode-light": "#FFF7E6", // Light amber
				"decode-text-light": "#D46B08",
				"execute-light": "#F9F0FF", // Light purple
				"execute-text-light": "#722ED1",
				"memory-light": "#E6FFFB", // Light teal
				"memory-text-light": "#08979C",
				"writeback-light": "#F6FFED", // Light green
				"writeback-text-light": "#389E0D",
				
				// Stage colors - Dark Mode
				"fetch-dark": "#111D2C", // Dark blue
				"fetch-text-dark": "#40A9FF",
				"decode-dark": "#2B1D11", // Dark amber
				"decode-text-dark": "#FFA940",
				"execute-dark": "#120338", // Dark purple
				"execute-text-dark": "#B37FEB",
				"memory-dark": "#0A2E35", // Dark teal
				"memory-text-dark": "#36CFC9",
				"writeback-dark": "#162312", // Dark green
				"writeback-text-dark": "#73D13D",
				
				stage: {
					cycle: '#6366f1',
					fetch: '#14b8a6',
					decode: '#f59e0b',
					execute: '#ec4899',
					memory: '#06b6d4',
					writeback: '#8b5cf6',
					exit: '#ef4444',
				},
				sidebar: {
					DEFAULT: 'hsl(var(--sidebar-background))',
					foreground: 'hsl(var(--sidebar-foreground))',
					primary: 'hsl(var(--sidebar-primary))',
					'primary-foreground': 'hsl(var(--sidebar-primary-foreground))',
					accent: 'hsl(var(--sidebar-accent))',
					'accent-foreground': 'hsl(var(--sidebar-accent-foreground))',
					border: 'hsl(var(--sidebar-border))',
					ring: 'hsl(var(--sidebar-ring))'
				}
			},
			borderRadius: {
				lg: 'var(--radius)',
				md: 'calc(var(--radius) - 2px)',
				sm: 'calc(var(--radius) - 4px)'
			},
			keyframes: {
				'accordion-down': {
					from: { height: '0' },
					to: { height: 'var(--radix-accordion-content-height)' }
				},
				'accordion-up': {
					from: { height: 'var(--radix-accordion-content-height)' },
					to: { height: '0' }
				},
				'fade-in': {
					from: { opacity: '0' },
					to: { opacity: '1' }
				},
				'fade-in-up': {
					from: { opacity: '0', transform: 'translateY(10px)' },
					to: { opacity: '1', transform: 'translateY(0)' }
				},
				'scale-in': {
					from: { opacity: '0', transform: 'scale(0.98)' },
					to: { opacity: '1', transform: 'scale(1)' }
				},
				'theme-switch': {
					'0%': { transform: 'scale(1) rotate(0deg)' },
					'50%': { transform: 'scale(1.1) rotate(20deg)' },
					'100%': { transform: 'scale(1) rotate(0deg)' }
				}
			},
			animation: {
				'accordion-down': 'accordion-down 0.2s ease-out',
				'accordion-up': 'accordion-up 0.2s ease-out',
				'fade-in': 'fade-in 0.3s ease-out',
				'fade-in-up': 'fade-in-up 0.4s ease-out',
				'scale-in': 'scale-in 0.2s ease-out',
				'theme-switch': 'theme-switch 0.5s ease-in-out'
			},
			fontFamily: {
				sans: [
					'Inter',
					'system-ui',
					'-apple-system',
					'BlinkMacSystemFont',
					'Segoe UI',
					'Roboto',
					'Helvetica Neue',
					'Arial',
					'sans-serif'
				]
			}
		}
	},
	plugins: [require("tailwindcss-animate")],
} satisfies Config;
