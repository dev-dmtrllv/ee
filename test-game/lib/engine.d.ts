
declare class Window
{
	protected constructor();
	public show(): void;
}

/**
 * TODO: implement on c++ side
 */
declare class Vector
{
	public get x(): number;
	public get y(): number;
	public get z(): number;

	public constructor();
}

declare class Component
{
	public readonly gameObject: GameObject;
}

declare module "*.png"
{
	const path: string;
	export default path;
}

declare module "*.jpg"
{
	const path: string;
	export default path;
}

type ComponentType<T extends Component> = new (...args: any[]) => T;

declare class Transform extends Component
{
	public get position(): Vector;
	public get scale(): Vector;
	public get rotation(): Vector;
}

declare class SpriteRenderer extends Component
{
	public set sprite(asset: Asset<SPRITE>);
}

declare class GameObject
{
	public readonly name: string;

	public get transform(): Transform;

	public constructor(name?: string, position?: Vector);

	public addComponent<T extends Component>(component: ComponentType<T>): T;
	public getComponent<T extends Component>(component: ComponentType<T>): T;
	public removeComponent<T extends Component>(component: T): boolean;
}

declare abstract class Scene
{
	public constructor();

	/**
	 * Here should all the assets that the current scene needs be loaded.
	 * You can think of all the textures, audio files, shaders, material files etc...
	 */
	protected abstract load(): void;

	/**
	 * When the scene is fully loaded the call method will be called.
	 * Here all the game objects are initialized with their coresponding components and data, so it's safe to access them.
	 */
	protected abstract start(): void;

	/**
	 * When another scene is loaded this scene's stop method will be called just before the other scene is started.
	 */
	protected abstract stop(): void;

	/**
	 * TODO: implement on c++ side
	 */
	protected spawn(name: string): GameObject;
	protected spawn(position: Vector): GameObject;
	protected spawn(name: string, position: Vector): GameObject;
}

type SceneType<T extends Scene = Scene> = new () => T;

declare var exports: { [key: string]: any; };

declare class Exception extends Error
{
	public constructor(name: string, msg: string);

	public toString();
}

declare namespace Engine
{
	class CoreException extends Exception
	{
		constructor(msg: string);
	}

	/**
	 * This holds the main game window.
	 */
	const window: Window;

	/**
	 * Call this to get an onLoadCallback.
	 * The function is used to configure the engine with it's game specific needs.
	 * @param onLoadCallback an async function to configure the engine
	 */
	const onLoad: (onLoadCallback: OnLoadCallback) => void;

	/**
	 * Log to the console
	 * @param args list of objects to log (it can be anything).
	 */
	const log: (...args: any[]) => void;

	/**
	 * Call this method when the game is configured. If not the engine will throw an Engine.CoreException.
	 * @param sceneName The name of the first scene to load.
	 */
	const start: (sceneName: string) => void;

	/**
	 * A method to retreive the current loaded (active) scene.
	 * @returns Scene or null if no scene is loaded.
	 */
	const getActiveScene: () => Scene | null;

}

type UnwrapPromise<T> = T extends Promise<infer P> ? P extends Promise<any> ? UnwrapPromise<P> : P : T;

declare type AUDIO = "AUDIO";
declare type SPRITE = "SPRITE";

declare type AssetTypes = AUDIO | SPRITE;

declare interface IAssetData
{
	get data(): Uint32Array;
}

declare class AudioData implements IAssetData
{
	get data(): Uint32Array;
	private constructor();
}

declare class SpriteData implements IAssetData
{
	get data(): Uint32Array;
	private constructor();
}

declare type AssetDataFromType<T> = T extends AUDIO ? AudioData : T extends SPRITE ? SpriteData : IAssetData;

declare class Asset<T = AssetTypes>
{
	public readonly type: T;

	private constructor();

	public get(): AssetDataFromType<T>;
}

declare namespace AssetManager
{
	const load: <T extends AssetTypes>(path: string) => Asset<T>;
}

type OnLoadCallback = (configure: EngineConfigureFunction) => void;

type EngineConfigureFunction = (config: EngineConfiguration) => Promise<void>;

type EngineConfiguration = {
	name: string;
	window?: WindowConfig;
	jobSystem?: JobSystemConfig;
	graphics?: GraphicsConfig;
	scenes?: {
		[name: string]: SceneType
	};
};

type Color = [number, number, number, number];

type GraphicsConfig = {
	clearColor?: Color;
};

type JobSystemConfig = {
	/**
	 * Total number of jobs to be executed.
	*/
	maxJobs?: number;
	/** 
	 * Total number of threads to execute the jobs. 
	 * */
	executionThreads?: number;
};

type WindowConfig = {
	/**
	 * The minimum with of the window.
	 */
	minWidth?: number;
	/**
	 * The minimum height of the window.
	 */
	minHeight?: number;
	/**
	 * The maximum with of the window.
	 */
	maxWidth?: number;
	/**
	 * The maximum height of the window.
	 */
	maxHeight?: number;
	/**
	 * The initial width of the window.
	 */
	width?: number;
	/**
	 * The initial height of the window.
	 */
	height?: number;
	/**
	 * Should the window be resizable?
	 */
	resizable?: boolean;
	/**
	 * Should the window start maximized?
	 */
	maximized?: boolean;
	/**
	 * Should the window start in fullscreen mode?
	 */
	fullscreen?: boolean;
	/**
	 * Should the window initially be hidden? 
	 */
	hidden?: boolean;
};
